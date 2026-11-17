#include <stddef.h>
#include <stdint.h>

#include "../../include/mb2.h"
#include "../mm/addr.h"
#include "../sync/spinlock.h"
#include "console.h"
#include "font.h"
#include "serial.h"

#define CELL_W 8
#define CELL_H 16

#define FG 0x00c8c8c8u
#define BG 0x00000000u

static uint32_t *fb;
static uint32_t pitch;
static uint32_t width, height;
static uint32_t col, row;
static spinlock_t lock = SPINLOCK_INIT;

static uint32_t cols(void) { return width / CELL_W; }

static uint32_t rows(void) { return height / CELL_H; }

static void draw_cell(uint32_t cx, uint32_t cy, char ch) {
    if (ch < 0x20 || ch > 0x7e)
        ch = '?';

    const unsigned char *glyph = font_glyphs[ch - 0x20];
    uint32_t stride = pitch / 4;
    uint32_t *line = fb + cy * CELL_H * stride + cx * CELL_W;

    for (uint32_t y = 0; y < CELL_H; y++) {
        unsigned char bits = glyph[y];
        for (uint32_t x = 0; x < CELL_W; x++)
            line[y * stride + x] = (bits & (0x80 >> x)) ? FG : BG;
    }
}

static void scroll(void) {
    uint32_t stride = pitch / 4;

    for (uint32_t y = 0; y + CELL_H <= height - CELL_H; y++)
        for (uint32_t x = 0; x < stride; x++)
            fb[y * stride + x] = fb[(y + CELL_H) * stride + x];

    for (uint32_t y = height - CELL_H; y < height; y++)
        for (uint32_t x = 0; x < stride; x++)
            fb[y * stride + x] = BG;
}

static void putc_locked(char c) {
    serial_putc(c);

    if (fb == NULL)
        return;

    if (c == '\n') {
        col = 0;
        row++;
    } else if (c == '\r') {
        col = 0;
    } else if (c == '\b') {
        if (col)
            col--;
        draw_cell(col, row, ' ');
    } else {
        draw_cell(col, row, c);
        if (++col >= cols()) {
            col = 0;
            row++;
        }
    }

    if (row >= rows()) {
        scroll();
        row = rows() - 1;
    }
}

void console_putc(char c) {
    uint64_t flags = spin_lock_irqsave(&lock);

    putc_locked(c);
    spin_unlock_irqrestore(&lock, flags);
}

void console_puts(const char *s) {
    uint64_t flags = spin_lock_irqsave(&lock);

    while (*s)
        putc_locked(*s++);
    spin_unlock_irqrestore(&lock, flags);
}

void console_puthex(uint64_t v) {
    static const char digits[] = "0123456789abcdef";
    uint64_t flags = spin_lock_irqsave(&lock);

    putc_locked('0');
    putc_locked('x');
    for (int i = 60; i >= 0; i -= 4)
        putc_locked(digits[(v >> i) & 0xf]);
    spin_unlock_irqrestore(&lock, flags);
}

void console_putdec(uint64_t v) {
    char buf[20];
    int i = 0;
    uint64_t flags = spin_lock_irqsave(&lock);

    do {
        buf[i++] = '0' + v % 10;
        v /= 10;
    } while (v);

    while (i)
        putc_locked(buf[--i]);
    spin_unlock_irqrestore(&lock, flags);
}

void console_init(const struct mb2_tag_framebuffer *t) {
    if (t == NULL || t->bpp != 32 || t->type != 1)
        return;

    fb = phys_to_virt(t->addr);
    pitch = t->pitch;
    width = t->width;
    height = t->height;
    col = row = 0;

    for (uint32_t i = 0; i < pitch / 4 * height; i++)
        fb[i] = BG;
}
