    .text
    .globl context_switch
context_switch:
    pushq   %rbx
    pushq   %rbp
    pushq   %r12
    pushq   %r13
    pushq   %r14
    pushq   %r15
    movq    %rsp, (%rdi)
    movq    %rsi, %rsp
    popq    %r15
    popq    %r14
    popq    %r13
    popq    %r12
    popq    %rbp
    popq    %rbx
    ret

    .globl thread_entry_stub
thread_entry_stub:
    movq    %r12, %rdi
    sti
    call    *%r13
    call    thread_exit
1:  hlt
    jmp     1b
