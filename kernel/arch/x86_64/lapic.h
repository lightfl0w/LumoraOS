#ifndef KERNEL_ARCH_LAPIC_H
#define KERNEL_ARCH_LAPIC_H

#include <stdint.h>

void lapic_init(void);
uint64_t lapic_timer_init(void);
void lapic_irq(uint32_t vector);
void lapic_wait_ticks(uint64_t n);

#endif
