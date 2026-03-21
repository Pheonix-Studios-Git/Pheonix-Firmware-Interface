/* PFI Firmware Linker Script (x64) */
OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

MEMORY
{
    /* Physical RAM at 1MB mark */
    RAM (rwx) : ORIGIN = 0x100000, LENGTH = 2M
    /* Flash ROM at top of 4GB */
    ROM (rx)  : ORIGIN = 0xFF000000, LENGTH = 16M
}

SECTIONS
{
    /* Page Tables & Low Data (RAM) */
    .data 0x100000 : AT(ADDR(.data)) { 
        *(.data*)
        *(.pagetables)
    } > RAM
    .bss : { *(.bss*) *(COMMON) } > RAM

    /* The PFI Runtime (Virtual High Half) */
    .text 0xFF000000 : AT(0) {
        *(.text.start)
        *(.text*)
        *(.rodata*)
    } > ROM

    /* The Reset Vector (The absolute end of 32-bit space) */
    .reset 0xFFFFFFF0 : AT(0xFFFFF0) {
        . = ALIGN(1);
        KEEP(*(.reset))
    } > ROM = 0x90
}