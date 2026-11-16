#include <stddef.h>
#include <stdint.h>

#include "../../include/mb2.h"
#include "page_alloc.h"

static uint64_t free_head;
static uint64_t free_count;
static uint64_t total_count;

extern char __kernel_phys_start[];
extern char __kernel_phys_end[];

static void push(uint64_t pa) {
    *(uint64_t *)pa = free_head;
    free_head = pa;
    free_count++;
}

void *page_alloc(void) {
    if (free_head == 0)
        return NULL;

    uint64_t pa = free_head;
    free_head = *(uint64_t *)pa;
    free_count--;
    return (void *)pa;
}

void page_free(void *page) { push((uint64_t)page); }

uint64_t page_total_count(void) { return total_count; }

uint64_t page_free_count(void) { return free_count; }

static void free_region(uint64_t start, uint64_t stop, uint64_t kstart,
                        uint64_t kend) {
    for (uint64_t pa = start; pa < stop; pa += PAGE_SIZE) {
        if (pa >= kstart && pa < kend)
            continue;
        push(pa);
        total_count++;
    }
}

static void free_mmap(const struct mb2_tag_mmap *m, uint32_t tag_size,
                      uint64_t kstart, uint64_t kend) {
    const uint8_t *e = (const uint8_t *)m->entries;
    uint32_t count =
        (m->entry_size > 0) ? (tag_size - sizeof(*m)) / m->entry_size : 0;

    for (uint32_t i = 0; i < count; i++, e += m->entry_size) {
        const struct mb2_mmap_entry *r = (const void *)e;

        if (r->type != MB2_MMAP_AVAILABLE)
            continue;

        free_region((r->addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1),
                    (r->addr + r->len) & ~(PAGE_SIZE - 1), kstart, kend);
    }
}

void page_alloc_init(const struct mb2_info *info) {
    const uint8_t *p = (const uint8_t *)info + sizeof(*info);
    const uint8_t *end = (const uint8_t *)info + info->total_size;
    uint64_t kstart = (uint64_t)__kernel_phys_start;
    uint64_t kend = (uint64_t)__kernel_phys_end;

    while (p + sizeof(struct mb2_tag) <= end) {
        const struct mb2_tag *tag = (const struct mb2_tag *)p;

        if (tag->type == MB2_TAG_END)
            break;
        if (tag->type == MB2_TAG_MMAP)
            free_mmap((const void *)tag, tag->size, kstart, kend);

        p += (tag->size + MB2_TAG_ALIGN - 1) & ~(MB2_TAG_ALIGN - 1);
    }
}
