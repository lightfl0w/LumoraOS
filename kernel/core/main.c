#include <stddef.h>
#include <stdint.h>

#include "../../include/mb2.h"
#include "../arch/x86_64/gdt.h"
#include "../arch/x86_64/idt.h"
#include "../drivers/serial.h"
#include "../mm/paging.h"

static void halt_forever(void)
{
    __asm__ volatile("cli; hlt");
    for (;;)
        ;
}

static void die(const char *msg)
{
    serial_puts("kernel: ");
    serial_puts(msg);
    serial_puts("\n");
    halt_forever();
}

static void dump_boot_info(const struct mb2_info *info)
{
    const uint8_t *p = (const uint8_t *)info + sizeof(*info);
    const uint8_t *end = (const uint8_t *)info + info->total_size;

    while (p + sizeof(struct mb2_tag) <= end) {
        const struct mb2_tag *tag = (const struct mb2_tag *)p;

        if (tag->type == MB2_TAG_END)
            break;

        if (tag->type == MB2_TAG_MMAP) {
            const struct mb2_tag_mmap *m = (const void *)tag;
            uint32_t count = (m->entry_size > 0)
                                 ? (tag->size - sizeof(*m)) / m->entry_size
                                 : 0;

            serial_puts("kernel: memory map entries: ");
            serial_puthex(count);
            serial_putc('\n');

            const uint8_t *e = (const uint8_t *)m->entries;
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

        p += (tag->size + MB2_TAG_ALIGN - 1) & ~(MB2_TAG_ALIGN - 1);
    }
}

void kmain(uint32_t magic, const struct mb2_info *info)
{
    serial_init();

    if (magic != MB2_BOOTLOADER_MAGIC)
        die("bad bootloader magic");

    if (info == NULL)
        die("no boot info");

    gdt_init();
    paging_init();
    idt_init();

    dump_boot_info(info);

    serial_puts("Hello, World!\n");
    halt_forever();
}
