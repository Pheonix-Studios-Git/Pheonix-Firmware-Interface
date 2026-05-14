#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

#include <core.h>
#include <vmem.h>
#include <serial.h>

#define VMEM 0xB8000
#define VMEM_MAX_COLS 80
#define VMEM_MAX_ROWS 25

static spinlock_t tmp_lock = 0;
static spinlock_t vmem_lock = 0;
static spinlock_t vmemf_lock = 0;
static spinlock_t vmemc_lock = 0;
static spinlock_t vmem_cur_lock = 0;

void vmem_set_cursor(uint16_t x, uint16_t y) {
    uint64_t rflags = spin_lock_irqsave(&vmem_cur_lock);

    uint16_t pos = y * VMEM_MAX_COLS + x;
    outb(0x3D4, 0x0E);
    outb(0x3D5, (pos >> 8) & 0xFF);
    outb(0x3D4, 0x0F);
    outb(0x3D5, pos & 0xFF);

    spin_unlock_irqrestore(&vmem_cur_lock, rflags);
}

void vmem_disable_cursor(void) {
    uint64_t rflags = spin_lock_irqsave(&vmem_cur_lock);

    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);

    spin_unlock_irqrestore(&vmem_cur_lock, rflags);
}

void vmem_clear_screen(struct VMemDesign* design) {
    uint64_t rflags = spin_lock_irqsave(&vmem_lock);

    volatile uint16_t* vmem = (volatile uint16_t*)VMEM;
    uint16_t attr = (design->bg << 4) | design->fg;
    for (uint16_t i = 0; i < VMEM_MAX_COLS*VMEM_MAX_ROWS; i++) vmem[i] = attr << 8;
    design->x = 0;
    design->y = 0;

    spin_unlock_irqrestore(&vmem_lock, rflags);
}

void vmem_printc(struct VMemDesign* design, char c) {
    uint64_t rflags = spin_lock_irqsave(&vmemc_lock);

    volatile uint16_t* vmem = (volatile uint16_t*)VMEM;
    uint16_t attr = (design->bg << 4) | design->fg;
    if (c == '\n') {
        design->x = 0;
        uint16_t y = design->y + 1;
        design->y = y;
        spin_unlock_irqrestore(&vmemc_lock, rflags);
        vmem_set_cursor(design->x, y);
        if (design->serial_out == 1) serial_printc(c);
        return;
    }
    vmem[design->y * VMEM_MAX_COLS + design->x] = ((uint16_t)attr << 8) | c;
    uint16_t x = design->x + 1;
    design->x = x;

    spin_unlock_irqrestore(&vmemc_lock, rflags);
    vmem_set_cursor(x, design->y);
    if (design->serial_out == 1) serial_printc(c);
}

void vmem_print(struct VMemDesign* design, const char* str) {\
    uint64_t rflags = spin_lock_irqsave(&vmem_lock);
    while (*str) vmem_printc(design, *str++);
    spin_unlock_irqrestore(&vmem_lock, rflags);
}

static void vmem_print_ex_integer(struct VMemDesign* design, uint64_t val, int base, int width, int zero_pad, int is_signed) {
    char buf[64];
    const char* digits = "0123456789abcdef";
    int i = 0;
    int neg = 0;
    if (is_signed && (int64_t)val < 0) {
        neg = 1;
        val = -(int64_t)val;
    }
    do {
        buf[i++] = digits[val % base];
        val /= base;
    } while (val > 0);

    int total_len = i + (neg ? 1 : 0);
    if (width > total_len) {
        int padding_count = width - total_len;
        if (zero_pad) {
            if (neg) {
                vmem_printc(design, '-');
                neg = 0;
            }
            while (padding_count--) vmem_printc(design, '0');
        } else {
            while(padding_count--) vmem_printc(design, ' ');
        }
    }

    if (neg) vmem_printc(design, '-');
    while (i > 0) {
        vmem_printc(design, buf[--i]);
    }
}

void vmem_printf(struct VMemDesign* design, const char* fmt, ...) {
    uint64_t rflags = spin_lock_irqsave(&vmemf_lock);

    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;

            int zero_pad = 0;
            int width = 0;
            int is_long = 0;

            if (*fmt == '0') {
                zero_pad = 1;
                fmt++;
            }
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }
            while (*fmt == 'l') {
                is_long++;
                fmt++;
            }

            switch (*fmt) {
                case 'c': {
                    char c = (char)va_arg(args, int);
                    vmem_printc(design, c);
                    break;
                }
                case 's': {
                    const char* s = va_arg(args, const char*);
                    vmem_print(design, s ? s : "(NULL)");
                    break;
                }
                case 'i':
                case 'd': { // signed 32/64-bit
                    int64_t d;
                    if (is_long >= 1) d = va_arg(args, int64_t);
                    else d = (int64_t)va_arg(args, int);
                    vmem_print_ex_integer(design, (uint64_t)d, 10, width, zero_pad, 1);
                    break;
                }
                case 'u': { // unsigned 32/64-bit
                    uint64_t u;
                    if (is_long >= 1) u = va_arg(args, uint64_t);
                    else u = (uint64_t)va_arg(args, uint32_t);
                    vmem_print_ex_integer(design, u, 10, width, zero_pad, 0);
                    break;
                }
                case 'x':
                case 'p': { // Pointer
                    uint64_t p;
                    if (*fmt == 'p') {
                        p = (uintptr_t)va_arg(args, void*);
                        serial_print("0x");
                        if (width == 0) width = 16;
                        zero_pad = 1;
                    } else {
                        if (is_long >= 1) p = va_arg(args, uint64_t);
                        else p = (uint64_t)va_arg(args, uint32_t);
                    }
                    vmem_print_ex_integer(design, p, 16, width, zero_pad, 0);
                    break;
                }
                case '%': {
                    vmem_printc(design, '%');
                    break;
                }
                default: {
                    vmem_printc(design, *fmt);
                    break;
                }
            }
        } else {
            vmem_printc(design, *fmt);
        }
        fmt++;
    }

    va_end(args);
    spin_unlock_irqrestore(&vmemf_lock, rflags);
}
void vmem_scroll_up(struct VMemDesign* design, uint32_t top, uint32_t bottom, uint32_t width) {
    uint16_t* vmem = (uint16_t*)VMEM;
    uint32_t stride = VMEM_MAX_COLS; 

    for (size_t y = top; y < bottom - 1; y++) {
        for (size_t x = 0; x < width; x++) {
            vmem[y * stride + x] = vmem[(y + 1) * stride + x];
        }
    }

    uint16_t blank_attr = (uint8_t)design->fg | (uint8_t)(design->bg << 4);
    uint16_t blank_char = (uint16_t)' ' | (uint16_t)(blank_attr << 8);

    for (size_t x = 0; x < width; x++) {
        vmem[(bottom - 1) * stride + x] = blank_char;
    }
}
