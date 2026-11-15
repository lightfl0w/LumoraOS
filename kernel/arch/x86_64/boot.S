    .section .text.entry, "ax"
    .globl _start
_start:
    cli
    xor     %ebp, %ebp
    mov     %eax, %edi
    mov     %ebx, %esi
    lea     boot_stack_top(%rip), %rsp
    call    kmain

    cli
1:  hlt
    jmp     1b

    .section .bss
    .align 16
boot_stack_bottom:
    .skip   16384
boot_stack_top:
