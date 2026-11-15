#include "serial.h"

#include "../arch/x86_64/io.h"

#define COM1 0x3f8

#define LSR_THRE 0x20

void serial_init(void)
{
    out8(COM1 + 1, 0x00); /* 不用 UART 中断 */
    out8(COM1 + 3, 0x80); /* DLAB 置位 */
    out8(COM1 + 0, 0x01); /* 115200 波特 */
    out8(COM1 + 1, 0x00);
    out8(COM1 + 3, 0x03); /* 8N1 */
    out8(COM1 + 2, 0xc7); /* FIFO */
    out8(COM1 + 4, 0x0b); /* DTR + RTS + OUT2 */
}

void serial_putc(char c)
{
    while (!(in8(COM1 + 5) & LSR_THRE))
        ;
    out8(COM1, (uint8_t)c);
}

void serial_puts(const char *s)
{
    while (*s)
        serial_putc(*s++);
}

void serial_puthex(uint64_t v)
{
    static const char digits[] = "0123456789abcdef";

    serial_puts("0x");
    for (int i = 60; i >= 0; i -= 4)
        serial_putc(digits[(v >> i) & 0xf]);
}

void serial_putdec(uint64_t v)
{
    char buf[20];
    int i = 0;

    do {
        buf[i++] = '0' + v % 10;
        v /= 10;
    } while (v);

    while (i)
        serial_putc(buf[--i]);
}
