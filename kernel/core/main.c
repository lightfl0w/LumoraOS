#include <stddef.h>
#include <stdint.h>

#include "../../include/mb2.h"
#include "../arch/x86_64/gdt.h"
#include "../arch/x86_64/idt.h"
#include "../arch/x86_64/lapic.h"
#include "../arch/x86_64/pic.h"
#include "../drivers/console.h"
#include "../drivers/serial.h"
#include "../mm/kmalloc.h"
#include "../mm/page_alloc.h"
#include "../mm/addr.h"
#include "../mm/paging.h"
#include "../sched/thread.h"

static void halt_forever(void) {
    __asm__ volatile("cli; hlt");
    for (;;)
        ;
}

static void die(const char *msg) {
    console_puts("kernel: ");
    console_puts(msg);
    console_puts("\n");
    halt_forever();
}

static const struct mb2_tag_framebuffer *find_fb(const struct mb2_info *info) {
    const uint8_t *p = (const uint8_t *)info + sizeof(*info);
    const uint8_t *end = (const uint8_t *)info + info->total_size;

    while (p + sizeof(struct mb2_tag) <= end) {
        const struct mb2_tag *tag = (const struct mb2_tag *)p;

        if (tag->type == MB2_TAG_FRAMEBUFFER)
            return (const void *)tag;
        if (tag->type == MB2_TAG_END)
            break;

        p += (tag->size + MB2_TAG_ALIGN - 1) & ~(MB2_TAG_ALIGN - 1);
    }
    return NULL;
}

static void dump_mmap(const struct mb2_tag_mmap *m, uint32_t tag_size) {
    uint32_t count =
        (m->entry_size > 0) ? (tag_size - sizeof(*m)) / m->entry_size : 0;
    const uint8_t *e = (const uint8_t *)m->entries;

    console_puts("kernel: memory map entries: ");
    console_puthex(count);
    console_putc('\n');

    for (uint32_t i = 0; i < count; i++, e += m->entry_size) {
        const struct mb2_mmap_entry *r = (const void *)e;

        if (r->type != MB2_MMAP_AVAILABLE)
            continue;
        console_puthex(r->addr);
        console_puts(" - ");
        console_puthex(r->addr + r->len);
        console_putc('\n');
    }
}

static void dump_boot_info(const struct mb2_info *info) {
    const uint8_t *p = (const uint8_t *)info + sizeof(*info);
    const uint8_t *end = (const uint8_t *)info + info->total_size;

    while (p + sizeof(struct mb2_tag) <= end) {
        const struct mb2_tag *tag = (const struct mb2_tag *)p;

        if (tag->type == MB2_TAG_END)
            break;
        if (tag->type == MB2_TAG_MMAP)
            dump_mmap((const void *)tag, tag->size);

        p += (tag->size + MB2_TAG_ALIGN - 1) & ~(MB2_TAG_ALIGN - 1);
    }
}

static void test_page_alloc(void) {
    void *a = page_alloc();
    void *b = page_alloc();

    if (a == NULL || b == NULL)
        die("page alloc exhausted");

    *(uint64_t *)phys_to_virt((uint64_t)a) = 0x6c756d6f7261ULL;
    if (*(uint64_t *)phys_to_virt((uint64_t)a) != 0x6c756d6f7261ULL)
        die("page not writable via physmap");

    page_free(b);
    page_free(a);
    void *c = page_alloc();

    if (c != a)
        die("page alloc selftest failed");
    page_free(c);

    console_puts("kernel: page alloc: ");
    console_putdec(page_total_count());
    console_puts(" pages (");
    console_putdec(page_free_count());
    console_puts(" free), selftest ok\n");
}

static void test_kmalloc(void) {
    char *p = kmalloc(100);
    char *q = kmalloc(100);

    if (p == NULL || q == NULL || p == q)
        die("kmalloc selftest failed");
    for (uint32_t i = 0; i < 100; i++)
        p[i] = (char)i;
    if (p[99] != 99)
        die("kmalloc not writable");
    kfree(p);
    kfree(q);

    if (kmalloc(0) != NULL || kmalloc(KMALLOC_MAX + 1) != NULL)
        die("kmalloc size check failed");

    char *r = kmalloc(100);

    if (r != q)
        die("kmalloc LIFO selftest failed");
    kfree(r);

    console_puts("kernel: kmalloc selftest ok\n");
}

static void worker(void *arg) {
    const char *name = arg;
    uint32_t n = 0;

    while (n < 4) {
        console_puts("<<");
        console_puts(name);
        console_putdec(++n);
        console_puts(">>");
        for (volatile uint32_t spin = 0; spin < 2000000; spin++)
            ;
    }
    console_puts("<");
    console_puts(name);
    console_puts(" done> ");
}

void kmain(uint32_t magic, const struct mb2_info *info) {
    serial_init();
    console_init(find_fb(info));

    if (magic != MB2_BOOTLOADER_MAGIC)
        die("bad bootloader magic");

    if (info == NULL)
        die("no boot info");

    info = phys_to_virt((uint64_t)info);

    gdt_init();
    paging_init(info);
    idt_init();
    page_alloc_init(info);

    pic_setup();
    lapic_init();
    uint64_t hz = lapic_timer_init();

    __asm__ volatile("sti");
    lapic_wait_ticks(3);

    console_puts("kernel: lapic timer ");
    console_putdec(hz);
    console_puts(" Hz, 3 ticks ok\n");

    test_page_alloc();
    test_kmalloc();
    dump_boot_info(info);

    sched_init();
    thread_create(worker, "A");
    thread_create(worker, "B");
    console_puts("\nsched: two threads running\n");

    for (;;) {
        __asm__ volatile("hlt");
        thread_yield();
    }
}
