#include <stddef.h>
#include <stdint.h>

#include "../../include/mb2.h"
#include "../arch/x86_64/gdt.h"
#include "../arch/x86_64/idt.h"
#include "../arch/x86_64/lapic.h"
#include "../arch/x86_64/pic.h"
#include "../drivers/serial.h"
#include "../mm/page_alloc.h"
#include "../mm/paging.h"

static void halt_forever(void) {
    __asm__ volatile("cli; hlt");
    for (;;)
        ;
}

static void die(const char *msg) {
    serial_puts("kernel: ");
    serial_puts(msg);
    serial_puts("\n");
    halt_forever();
}

static void dump_mmap(const struct mb2_tag_mmap *m, uint32_t tag_size) {
    uint32_t count =
        (m->entry_size > 0) ? (tag_size - sizeof(*m)) / m->entry_size : 0;
    const uint8_t *e = (const uint8_t *)m->entries;

    serial_puts("kernel: memory map entries: ");
    serial_puthex(count);
    serial_putc('\n');

    for (uint32_t i = 0; i < count; i++, e += m->entry_size) {
        const struct mb2_mmap_entry *r = (const void *)e;

        if (r->type != MB2_MMAP_AVAILABLE)
            continue;
        serial_puthex(r->addr);
        serial_puts(" - ");
        serial_puthex(r->addr + r->len);
        serial_putc('\n');
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

void kmain(uint32_t magic, const struct mb2_info *info) {
    serial_init();

    if (magic != MB2_BOOTLOADER_MAGIC)
        die("bad bootloader magic");

    if (info == NULL)
        die("no boot info");

    gdt_init();
    paging_init();
    idt_init();
    page_alloc_init(info);

    pic_setup();
    lapic_init();
    uint64_t hz = lapic_timer_init();

    __asm__ volatile("sti");
    lapic_wait_ticks(3);

    serial_puts("kernel: lapic timer ");
    serial_putdec(hz);
    serial_puts(" Hz, 3 ticks ok\n");

    /* 这里是用来自测的，之后可能会删掉 */
    void *a = page_alloc();
    void *b = page_alloc();

    if (a == NULL || b == NULL)
        die("page alloc exhausted");

    *(uint64_t *)a = 0x6c756d6f7261ULL;
    if (*(uint64_t *)a != 0x6c756d6f7261ULL)
        die("page not writable via identity map");

    page_free(b);
    page_free(a);
    void *c = page_alloc();

    if (c != a)
        die("page alloc selftest failed");
    page_free(c);

    serial_puts("kernel: page alloc: ");
    serial_putdec(page_total_count());
    serial_puts(" pages (");
    serial_putdec(page_free_count());
    serial_puts(" free), selftest ok\n");

    dump_boot_info(info);

    serial_puts("Hello, World!\n");
    halt_forever();
}
