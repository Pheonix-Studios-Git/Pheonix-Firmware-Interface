/* PFI Firmware Linker Script (x64) */
OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS
{
    .data 0x100000 : AT(SIZEOF(.text)) {
        _data_start = .;
        *(.data*)
        _data_end = .;
    }

    /* Page Tables & BSS (RAM) */
    .pagetables : AT(SIZEOF(.data) + ADDR(.data)) { *(.pagetables) }
    .bss : { *(.bss*) *(COMMON) }

    /* The PFI Runtime (Virtual High Half) */
    .text 0xFF000000 : AT(0) {
        *(.text.entry)
        *(.text*)
        *(.text.c)
        *(.rodata*)
        _etext = .;
    }

    /* The Reset Vector (The absolute end of 32-bit space) */
    . = 0xFFFFFFF0;
    .reset . : AT(0xFFFFF0) {
        KEEP(*(.reset))
    }
}