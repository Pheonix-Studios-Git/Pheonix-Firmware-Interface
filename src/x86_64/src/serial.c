#include <stdint.h>
#include <stdarg.h>

#include <core.h>
#include <serial.h>

#define SERIAL_PORT 0x3F8

static spinlock_t serial_lock = 0;
static spinlock_t serialf_lock = 0;
static spinlock_t serialc_lock = 0;
static spinlock_t serialp_lock = 0;

void serial_init(void) {
    uint64_t rflags = spin_lock_irqsave(&serial_lock);

    outb(0x00, SERIAL_PORT + 1); // Disable ints
    outb(0x80, SERIAL_PORT + 3); // Enable DLAB
    outb(0x03, SERIAL_PORT + 0); // Set baud rate divisor to 3 (38400)
    outb(0x00, SERIAL_PORT + 1);
    outb(0x03, SERIAL_PORT + 3); // 8 bits, no parity, one stop bit
    outb(0xC7, SERIAL_PORT + 2); // FIFO: enable, clear, 14-byte threshold
    outb(0x0B, SERIAL_PORT + 4); // IRQs enabled, RTS/DSR set

    spin_unlock_irqrestore(&serial_lock, rflags);
}

int serial_is_transmit_empty(void) {
    return inb(SERIAL_PORT + 5) & 0x20;
}

void serial_printc(char c) {
    uint64_t rflags = spin_lock_irqsave(&serialc_lock);
    while (!serial_is_transmit_empty());
    outb(c, SERIAL_PORT);
    spin_unlock_irqrestore(&serialc_lock, rflags);
}

void serial_print(const char* str) {
    uint64_t rflags = spin_lock_irqsave(&serialp_lock);
    while (*str) {
        if (*str == '\n') serial_printc('\r'); // Carriage return for terminals
        serial_printc(*str++);
    }
    spin_unlock_irqrestore(&serialp_lock, rflags);
}

static void serial_print_ex_integer(uint64_t val, int base, int width, int zero_pad, int is_signed) {
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
                serial_printc('-');
                neg = 0;
            }
            while (padding_count--) serial_printc('0');
        } else {
            while(padding_count--) serial_printc(' ');
        }
    }

    if (neg) serial_printc('-');
    while (i > 0) {
        serial_printc(buf[--i]);
    }
}

void serial_printf(const char* fmt, ...) {
    uint64_t rflags = spin_lock_irqsave(&serialf_lock);
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
                    serial_printc(c);
                    break;
                }
                case 's': {
                    const char* s = va_arg(args, const char*);
                    serial_print(s ? s : "(NULL)");
                    break;
                }
                case 'i':
                case 'd': { // signed 32/64-bit
                    int64_t d;
                    if (is_long >= 1) d = va_arg(args, int64_t);
                    else d = (int64_t)va_arg(args, int);
                    serial_print_ex_integer((uint64_t)d, 10, width, zero_pad, 1);
                    break;
                }
                case 'u': { // unsigned 32/64-bit
                    uint64_t u;
                    if (is_long >= 1) u = va_arg(args, uint64_t);
                    else u = (uint64_t)va_arg(args, uint32_t);
                    serial_print_ex_integer(u, 10, width, zero_pad, 0);
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
                    serial_print_ex_integer(p, 16, width, zero_pad, 0);
                    break;
                }
                case '%': {
                    serial_printc('%');
                    break;
                }
                default: {
                    serial_printc(*fmt);
                    break;
                }
            }
        } else {
            serial_printc(*fmt);
        }
        fmt++;
    }

    va_end(args);
    spin_unlock_irqrestore(&serialf_lock, rflags);
}
