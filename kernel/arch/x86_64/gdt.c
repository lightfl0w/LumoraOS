#include <stdint.h>

#include "gdt.h"

#define KERNEL_CODE 0x08
#define KERNEL_DATA 0x10

static uint64_t gdt[3] __attribute__((aligned(16))) = {
    0,
    0x00af9a000000ffff, /* L=1, P|S|EXEC|RW */
    0x00af92000000ffff, /* P|S|RW */
};

static struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdtr;

void gdt_init(void)
{
    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (uint64_t)gdt;

    __asm__ volatile(
        "lgdt %0\n"
        "pushq $0x08\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        "movw $0x10, %%dx\n"
        "movw %%dx, %%ds\n"
        "movw %%dx, %%es\n"
        "movw %%dx, %%ss\n"
        "xorl %%eax, %%eax\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        :
        : "m"(gdtr)
        : "rax", "rdx", "memory");
}
