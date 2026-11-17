    .section .text.boot, "ax"
    .globl _start
_start:
    cli
    mov     %eax, %edi
    mov     %ebx, %esi

    lea     tramp_pml4(%rip), %rax
    lea     tramp_pdpt_phys(%rip), %r11
    mov     %r11, %rcx
    or      $(1 | 2), %rcx
    mov     %rcx, (%rax)          /* 0 * 8 */

    lea     tramp_pdpt_high(%rip), %r10
    mov     %r10, %rcx
    or      $(1 | 2), %rcx
    mov     %rcx, 4088(%rax)      /* 511 * 8 */

    lea     tramp_pdpt_phys(%rip), %r11
    mov     %r11, %rcx
    or      $(1 | 2), %rcx
    mov     %rcx, 2184(%rax)      /* 273 * 8 */

    movq    $(1 | 2 | 0x80), 4080(%r10)

    xor     %r8d, %r8d
1:  mov     %r8, %r9
    shl     $30, %r9
    or      $(1 | 2 | 0x80), %r9
    mov     %r9, (%r11,%r8,8)
    inc     %r8
    cmp     $4, %r8
    jb      1b

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
tramp_pdpt_high:
    .skip   4096
    .align 4096
tramp_pdpt_phys:
    .skip   4096

    .section .bss
    .align 16
boot_stack_bottom:
    .skip   16384
boot_stack_top:
