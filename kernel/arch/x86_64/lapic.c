#include <stdint.h>

#include "../../drivers/pit.h"
#include "../../drivers/serial.h"
#include "lapic.h"
#include "msr.h"

#define MSR_APIC_BASE 0x1b

#define LAPIC_SPURIOUS 0x30
#define LAPIC_TIMER 0x20

#define TIMER_DIV_LOG2 4
#define TICK_HZ 100

#define REG_EOI 0x0b0
#define REG_SPURIOUS 0x0f0
#define REG_LVT_TIMER 0x320
#define REG_TIMER_ICR 0x380
#define REG_TIMER_CCR 0x390
#define REG_TIMER_DIV 0x3e0

#define LVT_MASK (1u << 16)
#define LVT_PERIODIC (1u << 17)

static volatile uint32_t *apic;
static uint64_t ticks;

static uint32_t apic_read(uint32_t reg) { return apic[reg / 4]; }

static void apic_write(uint32_t reg, uint32_t value) { apic[reg / 4] = value; }

static void die(const char *msg) {
    serial_puts("kernel: lapic: ");
    serial_puts(msg);
    serial_puts("\n");
    for (;;)
        __asm__ volatile("cli; hlt");
}

void lapic_init(void) {
    uint64_t base = rdmsr(MSR_APIC_BASE);

    if (!(base & (1ull << 11)))
        wrmsr(MSR_APIC_BASE, base | (1ull << 11));

    apic = (volatile uint32_t *)(base & 0xfffff000);

    apic_write(REG_LVT_TIMER, LAPIC_TIMER | LVT_MASK);
    apic_write(REG_SPURIOUS, LAPIC_SPURIOUS | (1u << 8));
}

uint64_t lapic_timer_init(void) {
    apic_write(REG_TIMER_DIV, 3); /* /16 */
    apic_write(REG_TIMER_ICR, 0xffffffffu);

    pit_wait_ms(10);

    uint64_t delta = 0xffffffffu - apic_read(REG_TIMER_CCR);
    uint64_t hz = delta * (1000 / 10);

    if (hz == 0)
        die("timer calibration failed");

    apic_write(REG_LVT_TIMER, LAPIC_TIMER | LVT_PERIODIC);
    apic_write(REG_TIMER_ICR, (uint32_t)(hz / TICK_HZ));

    return hz;
}

void lapic_irq(uint32_t vector) {
    if (vector == LAPIC_SPURIOUS)
        return;

    if (vector == LAPIC_TIMER)
        ticks++;

    apic_write(REG_EOI, 0);
}

void lapic_wait_ticks(uint64_t n) {
    uint64_t target = ticks + n;

    while (ticks < target)
        __asm__ volatile("hlt");
}
