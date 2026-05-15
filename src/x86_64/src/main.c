#include <stdint.h>

#include <core.h>
#include <builtins.h>

#include <serial.h>
#include <e820.h>
#include <acpi.h>
#include <idt.h>
#include <pcie.h>
#include <module.h>
#include <smp.h>

#include <pager.h>
#include <avmf.h>

__attribute__((used, noinline, section(".text.c"), noreturn)) void pfi_main(void) {
    idt_init();

    serial_init();
    serial_print("PFI Bootup successful!\n");

    init_basic_e820();
    pager_init();

    idt_global_init(); // Allow SMP working

    acpi_init();
    smp_init();

    pcie_init();

    for (;;) asm("hlt");
}