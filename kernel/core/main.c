#include "../../include/mb2.h"
#include "../drivers/serial.h"
#include <stddef.h>
#include <stdint.h>

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

static void puts_hex64(uint64_t v) {
    static const char digits[] = "0123456789abcdef";

    serial_puts("0x");
    for (int i = 60; i >= 0; i -= 4)
        serial_putc(digits[(v >> i) & 0xf]);
}

static void dump_boot_info(const struct mb2_info *info) {
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
            puts_hex64(count);
            serial_putc('\n');

            const uint8_t *e = (const uint8_t *)m->entries;
            for (uint32_t i = 0; i < count; i++, e += m->entry_size) {
                const struct mb2_mmap_entry *r = (const void *)e;

                if (r->type != MB2_MMAP_AVAILABLE)
                    continue;
                puts_hex64(r->addr);
                serial_puts(" - ");
                puts_hex64(r->addr + r->len);
                serial_putc('\n');
            }
        } else if (tag->type == MB2_TAG_FRAMEBUFFER) {
            const struct mb2_tag_framebuffer *fb = (const void *)tag;

            serial_puts("kernel: framebuffer: ");
            puts_hex64(fb->width);
            serial_putc('x');
            puts_hex64(fb->height);
            serial_puts(" @ ");
            puts_hex64(fb->addr);
            serial_putc('\n');
        }

        p += (tag->size + MB2_TAG_ALIGN - 1) & ~(MB2_TAG_ALIGN - 1);
    }
}

void kmain(uint32_t magic, const struct mb2_info *info) {
    serial_init();

    if (magic != MB2_BOOTLOADER_MAGIC)
        die("bad bootloader magic");

    if (info == NULL)
        die("no boot info");

    dump_boot_info(info);

    serial_puts("Hello, World!\n");
    halt_forever();
}
