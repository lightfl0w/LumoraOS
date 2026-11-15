#include <stdint.h>

#include "../drivers/serial.h"
#include "paging.h"

#define PTE_PRESENT (1ull << 0)
#define PTE_WRITE   (1ull << 1)
#define PTE_HUGE    (1ull << 7) 

#define CR4_LA57 (1ull << 12)

static uint64_t pml4[512] __attribute__((aligned(4096)));
static uint64_t pdpt[512] __attribute__((aligned(4096)));

static void die(const char *msg)
{
    serial_puts("kernel: paging: ");
    serial_puts(msg);
    serial_puts("\n");
    for (;;)
        __asm__ volatile("cli; hlt");
}

void paging_init(void)
{
    uint64_t cr4;

    __asm__ volatile("movq %%cr4, %0" : "=r"(cr4));
    if (cr4 & CR4_LA57)
        die("5-level paging unsupported");

    pml4[0] = (uint64_t)pdpt | PTE_PRESENT | PTE_WRITE;
    for (int i = 0; i < 4; i++)
        pdpt[i] = ((uint64_t)i << 30) | PTE_PRESENT | PTE_WRITE | PTE_HUGE;

    __asm__ volatile("movq %0, %%cr3" :: "r"((uint64_t)pml4) : "memory");
}
