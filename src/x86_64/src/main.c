#include <stdint.h>

__attribute__((used, noinline, section(".text.start"), noreturn)) void pfi_main(void) {
    volatile uint16_t *video = (uint16_t*)0xB8000;
    *video = (uint16_t)('A' | (0x07 << 8));
    for (;;) asm("hlt");
}