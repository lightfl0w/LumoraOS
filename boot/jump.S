    .text
    .intel_syntax noprefix
    .globl boot_jump
boot_jump:
    mov     rsp, r8
    xor     ebp, ebp
    mov     eax, 0x36d76289     /* MB2_BOOTLOADER_MAGIC */
    mov     ebx, edx
    jmp     rcx
