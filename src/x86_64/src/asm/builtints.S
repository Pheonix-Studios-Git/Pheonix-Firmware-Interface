/* Pheonix Firmware Interface - x86_64 Builtins */

.section .text
    .global memcpy
    .global memcmp
    .global memmove

    .global strlen
    .global strcmp
    .global strcpy

memcpy: // void memcpy(void* d, void* src, size_t n);
    movq %rdx, %rcx
    cld
    rep movsb

    ret

memcmp: // int memcmp(void* a, void* b)
    test %rdx, %rdx
    je .memcmp_eq
    .memcmp_loop:
        movzbl (%rdi), %eax
        movzbl (%rsi), %edx
        cmp %edx, %eax
        jne .memcmp_diff
        inc %rdi
        inc %rsi
        dec %rdx
        jnz .memcmp_loop
    .memcmp_eq:
        xor %eax, %eax

        ret
    .memcmp_diff:
        sub %edx, %eax

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
