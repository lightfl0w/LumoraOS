    .section .text

    .macro ISR, n
isr_\n:
    .if \n == 8 || \n == 10 || \n == 11 || \n == 12 || \n == 13 || \n == 14 || \n == 17 || \n == 21
    .else
    pushq $0
    .endif
    pushq $\n
    jmp isr_common
    .endm

    .irp n, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    ISR \n
    .endr

    ISR 32
    ISR 48

isr_common:
    pushq 40(%rsp)      /* 发生故障时的 rsp */
    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12
    pushq %r11
    pushq %r10
    pushq %r9
    pushq %r8
    pushq %rdi
    pushq %rsi
    pushq %rbp
    pushq %rbx
    pushq %rdx
    pushq %rcx
    pushq %rax

    cld
    movq %rsp, %rdi
    call x86_interrupt

    popq %rax
    popq %rcx
    popq %rdx
    popq %rbx
    popq %rbp
    popq %rsi
    popq %rdi
    popq %r8
    popq %r9
    popq %r10
    popq %r11
    popq %r12
    popq %r13
    popq %r14
    popq %r15
    addq $24, %rsp
    iretq

    .section .rodata
    .globl isr_stubs

    isr_stubs:
    .irp n, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    .quad isr_\n
    .endr
    .quad isr_32
    .quad isr_48
