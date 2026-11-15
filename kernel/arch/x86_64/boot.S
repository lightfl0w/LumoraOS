    .section .text.boot, "ax"
    .globl _start
_start:
    cli
    mov     %eax, %edi
    mov     %ebx, %esi

    lea     tramp_pml4(%rip), %rax
    lea     tramp_pdpt(%rip), %r10
    mov     %r10, %rcx
    or      $(1 | 2), %rcx        /* P | W */
    mov     %rcx, (%rax)          /* pml4[0] */
    mov     %rcx, 4088(%rax)      /* pml4[511] */

    xor     %r8d, %r8d
1:  mov     %r8, %r9
    shl     $30, %r9
    or      $(1 | 2 | 0x80), %r9  /* P | W | PS */
    mov     %r9, (%r10,%r8,8)
    inc     %r8
    cmp     $4, %r8
    jb      1b

    movq    $(1 | 2 | 0x80), 4080(%r10)

    lea     tramp_pml4(%rip), %rax
    mov     %rax, %cr3

    movabsq $_start_high, %rax
    jmp     *%rax

    .text
    .globl _start_high
_start_high:
    xor     %ebp, %ebp
    lea     boot_stack_top(%rip), %rsp
    call    kmain

    cli
1:  hlt
    jmp     1b

    .section .boot.data, "aw"
    .align 4096
tramp_pml4:
    .skip   4096
    .align 4096
tramp_pdpt:
    .skip   4096

    .section .bss
    .align 16
boot_stack_bottom:
    .skip   16384
boot_stack_top:
