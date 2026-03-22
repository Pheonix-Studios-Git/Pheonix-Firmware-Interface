#include <stdint.h>

#include <serial.h>
#include <core.h>

__attribute__((used, noinline, section(".text.c"), noreturn)) void pfi_main(void) {
    serial_init();

    serial_print("PFI Bootup successful!\n");

    for (;;) asm("hlt");
}