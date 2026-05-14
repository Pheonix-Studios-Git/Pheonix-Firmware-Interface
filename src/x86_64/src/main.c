#include <stdint.h>

#include <core.h>
#include <builtins.h>

#include <serial.h>
#include <e820.h>

#include <pager.h>
#include <avmf.h>

__attribute__((used, noinline, section(".text.c"), noreturn)) void pfi_main(void) {
    serial_init();
    serial_print("PFI Bootup successful!\n");
    
    init_basic_e820();
    
    pager_init();

    for (;;) asm("hlt");
}