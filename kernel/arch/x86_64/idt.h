#ifndef KERNEL_ARCH_IDT_H
#define KERNEL_ARCH_IDT_H

struct frame {
    uint64_t rax, rcx, rdx, rbx, rbp, rsi, rdi;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rsp;
    uint64_t vector, error;
    uint64_t rip, cs, rflags, rsp3, ss;
};

void idt_init(void);
void x86_interrupt(const struct frame *f);

#endif
