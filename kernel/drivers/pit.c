#include "pit.h"

#include "../arch/x86_64/io.h"

#define PIT_HZ 1193182

void pit_wait_ms(uint64_t ms) {
    uint16_t count = (uint16_t)(PIT_HZ / 1000 * ms);

    out8(0x61, (in8(0x61) & ~0x02) | 0x01);

    out8(0x43, 0xb0);
    out8(0x42, count & 0xff);
    out8(0x42, count >> 8);

    while (!(in8(0x61) & 0x20))
        ;
}
