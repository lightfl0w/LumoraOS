#ifndef KERNEL_DRIVERS_CONSOLE_H
#define KERNEL_DRIVERS_CONSOLE_H

#include <stdint.h>

struct mb2_tag_framebuffer;

void console_init(const struct mb2_tag_framebuffer *fb);
void console_putc(char c);
void console_puts(const char *s);
void console_puthex(uint64_t v);
void console_putdec(uint64_t v);

#endif
