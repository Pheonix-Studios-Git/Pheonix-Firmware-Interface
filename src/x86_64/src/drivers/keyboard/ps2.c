#include <core.h>
#include <ps2.h>
#include <vmem.h>

static spinlock_t tmp_lock = 0;
static spinlock_t ps2_lock = 0;

// Scan Codes
static const char scan_to_ascii[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0
};

int is_ps2_present(void) {
    uint64_t rflags = spin_lock_irqsave(&ps2_lock);

    int out = 0;
    while (inb(0x64) & 0x01) { (void)inb(0x60); }
    outb(0x60, 0xEE);
    for (int i = 0; i < 500000; i++) {
        if (inb(0x64) & 0x01) {
            uint8_t resp = inb(0x60);
            if (resp == 0xEE) { out = 1; break; } // keyboard present
            else { out = 0; break; };
        }
    }

    spin_unlock_irqrestore(&ps2_lock, rflags);
    return out;
}

void ps2_init(void) {
    uint64_t rflags = spin_lock_irqsave(&ps2_lock);

    uint8_t config;
    while (inb(0x64) & 0x02) { asm volatile("pause"); }
    outb(0x64, 0x20);
    while (!(inb(0x64) & 0x01)) {}
    config = inb(0x60);

    // Set bits: bit 0 = interrupt on keyboard, bit 4 = enable keyboard port
    config |= (1 << 0) | (1 << 4);

    // Write config byte back
    while (inb(0x64) & 0x02) { asm volatile("pause"); }
    outb(0x64, 0x60);
    while (inb(0x64) & 0x02) { asm volatile("pause"); }
    outb(0x60, config);

    // Send enable scanning command to keyboard
    while (inb(0x64) & 0x02) { asm volatile("pause"); }
    outb(0x60, 0xF4);

    spin_unlock_irqrestore(&ps2_lock, rflags);
}

int8_t ps2_read_scan(void) {
    uint64_t rflags = spin_lock_irqsave(&ps2_lock);

    while (!(inb(0x64) & 1)); // wait for data
    int8_t out = inb(0x60);
    spin_unlock_irqrestore(&ps2_lock, rflags);

    return out;
}

int16_t ps2_try_read_scan(void) {
    uint64_t rflags = spin_lock_irqsave(&ps2_lock);
    
    if (!(inb(0x64) & 1)) {
        spin_unlock_irqrestore(&ps2_lock, rflags);
        return -1;
    }
    int16_t out = (int16_t)inb(0x60);
    spin_unlock_irqrestore(&ps2_lock, rflags);

    return out;
}

void ps2_read_line(char* buf, int max_len, struct VMemDesign* design) {
    int idx = 0;
    for (;;) {
        uint8_t sc = ps2_read_scan(); // read a scan code
        char c = 0;

        // Ignore key releases (high bit set)
        if (sc & 0x80) continue;

        c = scan_to_ascii[sc];
        if (!c) continue;

        if (c == '\n') { // Enter
            buf[idx] = 0;
            vmem_printc(design, '\n');
            break;
        } else if (c == '\b') { // Backspace
            if (idx > 0) {
                idx--;
                // Move cursor back visually
                if (design->x == 0 && design->y > 0) {
                    design->y--;
                    design->x = VMEM_MAX_COLS - 1;
                } else if (design->x > 0) {
                    design->x--;
                }
                vmem_set_cursor(design->x, design->y);
                vmem_printc(design, ' '); // erase
                design->x--;
                vmem_set_cursor(design->x, design->y);
            }
        } else {
            if (idx < max_len - 1) {
                buf[idx++] = c;
                vmem_printc(design, c); // echo typed char
            }
        }
    }
}

