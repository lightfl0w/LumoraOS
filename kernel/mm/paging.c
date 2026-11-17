
#include <stddef.h>
#include <stdint.h>

#include "../../include/mb2.h"
#include "../drivers/serial.h"
#include "addr.h"
#include "paging.h"

#define PTE_PRESENT (1ull << 0)
#define PTE_WRITE   (1ull << 1)
#define PTE_USER    (1ull << 2)
#define PTE_HUGE    (1ull << 7)
#define PTE_NX      (1ull << 63)

#define CR4_LA57 (1ull << 12)

#define PAGE_2M     0x200000ull
#define PDPT_ENTRIES 512u

static uint64_t pd_pool[PDPT_ENTRIES][512] __attribute__((aligned(4096)));

static uint64_t pml4[512] __attribute__((aligned(4096)));
static uint64_t pdpt_high[512] __attribute__((aligned(4096)));
static uint64_t pdpt_phys[512] __attribute__((aligned(4096)));

extern char __text_start[], __text_end[];
extern char __rodata_start[], __rodata_end[];
extern char __data_start[], __bss_end[];

static void die(const char *msg)
{
    serial_puts("kernel: paging: ");
    serial_puts(msg);
    serial_puts("\n");
    for (;;)
        __asm__ volatile("cli; hlt");
}

static void enable_nx(void)
{
    const uint32_t msr_efer = 0xc0000080;
    const uint64_t nxe = 1ull << 11;
    uint32_t lo, hi;

    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr_efer));
    if ((((uint64_t)hi << 32) & nxe) == 0)
        __asm__ volatile("wrmsr"
                         :
                         : "c"(msr_efer), "a"(lo), "d"(hi | (uint32_t)(nxe >> 32))
                         : "memory");
}

static uint64_t kpd[512] __attribute__((aligned(4096)));

static uint64_t kpt_pool[8][512] __attribute__((aligned(4096)));
static uint32_t kpt_used;

static int32_t kpt_owner[512] __attribute__((aligned(4096)));

static uint64_t *kpt_for(uint32_t pd_i)
{
    if (kpt_owner[pd_i] < 0) {
        if (kpt_used >= 8)
            return NULL;
        kpt_owner[pd_i] = (int32_t)kpt_used;
        kpd[pd_i] = kernel_virt_to_phys(kpt_pool[kpt_used]) | PTE_PRESENT |
                    PTE_WRITE;
        kpt_used++;
    }
    return kpt_pool[kpt_owner[pd_i]];
}

static void map_kernel_page(uint64_t va, uint64_t flags)
{
    uint32_t pd_i = (va >> 21) & 511;
    uint32_t pt_i = (va >> 12) & 511;
    uint64_t *pt = kpt_for(pd_i);

    if (pt == NULL)
        die("kernel mapping needs more PTs");
    pt[pt_i] = (va - KERNEL_BASE) | PTE_PRESENT | flags;
}

static void map_kernel_image(void)
{
    for (uint32_t i = 0; i < 512; i++)
        kpt_owner[i] = -1;

    uint64_t start = (uint64_t)__text_start & ~0xfffull;
    uint64_t end = ((uint64_t)__bss_end + 0xfff) & ~0xfffull;

    for (uint64_t va = start; va < end; va += 0x1000)
        map_kernel_page(va, PTE_WRITE | PTE_NX);

    for (uint64_t va = (uint64_t)__text_start; va < (uint64_t)__text_end;
         va += 0x1000)
        map_kernel_page(va, 0); /* RX */

    for (uint64_t va = (uint64_t)__rodata_start; va < (uint64_t)__rodata_end;
         va += 0x1000)
        map_kernel_page(va, PTE_NX); /* RO+NX */

    pdpt_high[510] = kernel_virt_to_phys(kpd) | PTE_PRESENT | PTE_WRITE;
}

static void map_region(uint64_t base, uint64_t size)
{
    uint64_t start = base & ~(PAGE_2M - 1);
    uint64_t end = (base + size + PAGE_2M - 1) & ~(PAGE_2M - 1);

    for (uint64_t pa = start; pa < end; pa += PAGE_2M) {
        uint64_t va = PHYSMAP_BASE + pa;
        uint32_t pdpt_i = (va >> 30) & 511;
        uint32_t pd_i = (va >> 21) & 511;

        if (!(pdpt_phys[pdpt_i] & PTE_PRESENT))
            pdpt_phys[pdpt_i] = kernel_virt_to_phys(pd_pool[pdpt_i]) |
                                PTE_PRESENT | PTE_WRITE;

        pd_pool[pdpt_i][pd_i] =
            pa | PTE_PRESENT | PTE_WRITE | PTE_HUGE | PTE_NX;
    }
}

static void map_all_ram(const struct mb2_info *info)
{
    const uint8_t *p = (const uint8_t *)info + sizeof(*info);
    const uint8_t *end = (const uint8_t *)info + info->total_size;

    while (p + sizeof(struct mb2_tag) <= end) {
        const struct mb2_tag *tag = (const struct mb2_tag *)p;

        if (tag->type == MB2_TAG_END)
            break;
        if (tag->type == MB2_TAG_MMAP) {
            const struct mb2_tag_mmap *m = (const void *)tag;
            const uint8_t *e = (const uint8_t *)m->entries;
            uint32_t count = (m->entry_size > 0)
                                 ? (tag->size - sizeof(*m)) / m->entry_size
                                 : 0;

            for (uint32_t i = 0; i < count; i++, e += m->entry_size) {
                const struct mb2_mmap_entry *r = (const void *)e;

                if (r->type == MB2_MMAP_AVAILABLE)
                    map_region(r->addr, r->len);
            }
        }

        p += (tag->size + MB2_TAG_ALIGN - 1) & ~(MB2_TAG_ALIGN - 1);
    }
}

void paging_init(const struct mb2_info *info)
{
    uint64_t cr4;

    __asm__ volatile("movq %%cr4, %0" : "=r"(cr4));
    if (cr4 & CR4_LA57)
        die("5-level paging unsupported");

    enable_nx();

    pml4[511] = kernel_virt_to_phys(pdpt_high) | PTE_PRESENT | PTE_WRITE;
    pml4[(PHYSMAP_BASE >> 39) & 511] =
        kernel_virt_to_phys(pdpt_phys) | PTE_PRESENT | PTE_WRITE;

    map_kernel_image();
    map_all_ram(info);

    map_region(0, 0x100000000ull);

    __asm__ volatile("movq %0, %%cr3" ::"r"(kernel_virt_to_phys(pml4))
                     : "memory");
}
