/* Pheonix Firmware Interface - x86_64 Builtins */

.section .text
    .global memcpy
    .global memcmp
    .global memmove
    .global memset

    .global strlen
    .global strcmp
    .global strcpy

memcpy: // void memcpy(void* d, void* src, size_t n);
    movq %rdx, %rcx
    cld
    rep movsb

    ret

memcmp: // int memcmp(void* a, void* b, size_t n)
    test %rdx, %rdx
    je .memcmp_eq
    .memcmp_loop:
        movzbl (%rdi), %eax
        movzbl (%rsi), %ecx

        cmp %ecx, %eax
        jne .memcmp_diff

        inc %rdi
        inc %rsi
        dec %rdx
        jnz .memcmp_loop
    .memcmp_eq:
        xor %eax, %eax
        ret
    .memcmp_diff:
        sub %ecx, %eax
        ret

memmove: // void memmove(void* dst, void* src)
    cmp %rdi, %rsi
    ja .memmove_forward
    # backward copy
    lea -1(%rdi,%rdx), %rdi
    lea -1(%rsi,%rdx), %rsi
    std
    rep movsb
    cld

    ret
    .memmove_forward:
        cld
        rep movsb

        ret

memset: // void* memset(void* dst, int c, size_t n)
    mov %rdi, %rax
    test %rdx, %rdx
    je .memset_done

    movzbq %sil, %rcx
    
    mov %rcx, %rax
    shl $8,  %rcx
    or  %rax, %rcx

    mov %rcx, %rax
    shl $16, %rcx
    or  %rax, %rcx

    mov %rcx, %rax
    shl $32, %rcx
    or  %rax, %rcx

    mov %rdx, %r8
    shr $3, %rdx
    cld
    rep stosq

    mov %r8, %rdx 
    and $7, %rdx 
    mov %cl, %al
    rep stosb
    .memset_done:
        ret

strlen: // size_t strlen(char* a)
    xor %rax, %rax
    .strlen_loop:
        movzbl (%rdi,%rax,1), %edx
        test %dl, %dl
        je .strlen_done
        inc %rax
        jmp .strlen_loop
    .strlen_done:
        ret

strcmp: // int strcmp(char* a, char* b)
    .strcmp_loop:
        movzbl (%rdi), %eax
        movzbl (%rsi), %edx
        cmp %edx, %eax
        jne .strcmp_diff
        test %al, %al
        je .strcmp_eq
        inc %rdi
        inc %rsi
        jmp .strcmp_loop
    .strcmp_diff:
        sub %edx, %eax

        ret
    .strcmp_eq:
        xor %eax, %eax

        ret

strcpy: // void strcpy(char* dst, char* src)
    mov %rdi, %rax
    .strcpy_loop:
        movb (%rsi), %dl
        movb %dl, (%rdi)
        inc %rsi
        inc %rdi
        test %dl, %dl
        jnz .strcpy_loop

        ret
