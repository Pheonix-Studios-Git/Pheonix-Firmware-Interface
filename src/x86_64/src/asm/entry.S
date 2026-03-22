/* Phoenix Firmware Interface - x86_64 Entry */
.equ PAGE_PRESENT, (1 << 0)
.equ PAGE_RW, (1 << 1)
.equ PAGE_PS, (1 << 7) // Huge page (2MB in PD)

.extern pfi_main

.section .text.entry, "ax"
    .code16
early_init:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %ss
    movw $0x7C00, %sp

    inb $0x92, %al
    orb $2, %al
    outb %al, $0x92
    
    // Load 32-bit GDT
    pushl $0x100000 // Base address of .data
    pushw $0x17 // Limit (3 entries * 8 - 1)
    movl %esp, %ebx
    addr32 data32 lgdt (%ebx) // Wierd fix to make this work due to linker HATING IT
    addl $6, %esp // Clean up stack
    
    // Transition to Protected Mode
    movl %cr0, %eax
    orl $1, %eax
    movl %eax, %cr0
    
    // SEE THIS IS WHAT I HAVE TO DO BECAUSE OF ASSEMBLERS AND LINKERS WHEN WORKING IN THIS ENVIRONMENT
    // YEAH THIS IS RAW MACHINE CODE FOR x86_64 IN THE ASSEMBLY FILE ITSELF
    
    .byte 0x66
    .byte 0xEA // jmp 0x08:<addr>
    .long 0xFF000000 + (init_pm - early_init)
    .word 0x08

.code32
init_pm:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %ss
    movw %ax, %es

    // Setup Paging
    call clear_page_tables
    call setup_paging_structures

    // Enable PAE
    movl %cr4, %eax
    orl $(1 << 5), %eax
    movl %eax, %cr4

    // Load PML4 into CR3
    movl $pml4, %eax
    movl %eax, %cr3

    // Enable Long Mode in EFER MSR
    movl $0xC0000080, %ecx
    rdmsr
    orl $(1 << 8), %eax
    wrmsr

    // Enable Paging & Long Mode
    movl %cr0, %eax
    orl $(1 << 31), %eax
    movl %eax, %cr0

    // Load 64-bit GDT and Jump
    lgdt (gdt64_descriptor)

    movl $0xFF000000, %eax
    pushl $0x08 // CS
    pushl %eax // EIP/RIP
    lretl

    .byte 0xEA
    .long init_pm64
    .byte 0x08 // jmp 0x08:init_pm64

clear_page_tables:
    // Zero out PML4, PDPT, and PD0
    movl $pml4, %edi
    movl $4096, %ecx // (4096 * 4) / 4
    xorl %eax, %eax
    rep stosl
    ret

setup_paging_structures:
    // PML4[0] -> PDPT
    movl $pdpt, %eax
    orl $(PAGE_PRESENT | PAGE_RW), %eax
    movl %eax, (pml4)

    // Identity Map 0-1GB (PDPT[0] -> PD0)
    movl $pd0, %eax
    orl $(PAGE_PRESENT | PAGE_RW), %eax
    movl %eax, (pdpt)

    movl $pd0, %edi
    movl $0, %ebx
    movl $512, %ecx
.fill_pd0:
    movl %ebx, %eax
    orl $(PAGE_PRESENT | PAGE_RW | PAGE_PS), %eax
    movl %eax, (%edi)
    movl $0, 4(%edi)
    addl $8, %edi
    addl $0x200000, %ebx
    decl %ecx
    jnz .fill_pd0

    // Identity Map 3GB-4GB (PDPT[3] -> PD1) for Reset Vector or PFI Entry
    movl $pd1, %eax
    orl $(PAGE_PRESENT | PAGE_RW), %eax
    movl %eax, pdpt + (3 * 8)

    movl $pd1, %edi
    movl $0xC0000000, %ebx
    movl $512, %ecx
.fill_pd1:
    movl %ebx, %eax
    orl $(PAGE_PRESENT | PAGE_RW | PAGE_PS), %eax
    movl %eax, (%edi)
    movl $0, 4(%edi)
    addl $8, %edi
    addl $0x200000, %ebx
    decl %ecx
    jnz .fill_pd1
    ret

.code64
init_pm64:
    // PFI Entry setup
    movq $0x80000, %rsp
    movq %rsp, %rbp
    movabs $pfi_main, %rax
    jmp *%rax

.section .reset, "ax"
    .code16
    .global _start
_start:
    .byte 0x66
    .byte 0xEA
    .long early_init
    .word 0x00

    .p2align
    .org 16

.section .data
    .align 16
gdt_start:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF // 32-bit Code
    .quad 0x00CF92000000FFFF // 32-bit Data
gdt_descriptor:
    .word . - gdt_start - 1
    .long gdt_start

gdt64_start:
    .quad 0x0000000000000000
    .quad 0x00AF9A000000FFFF // 64-bit Code
    .quad 0x00AF92000000FFFF // 64-bit Data
gdt64_descriptor:
    .word . - gdt64_start - 1
    .quad gdt64_start

.section .pagetables
.align 4096
pml4: .skip 4096
pdpt: .skip 4096
pd0: .skip 4096
pd1: .skip 4096