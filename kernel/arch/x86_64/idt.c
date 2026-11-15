#include <stdint.h>

#include "../../drivers/serial.h"
#include "idt.h"

/* P=1, DPL=0, TYPE=0xE */
#define GATE_INTR 0x8e00

#define IDT_ENTRIES 256

struct idt_entry {
    uint16_t off_lo;
    uint16_t selector;
    uint16_t ist_type;
    uint16_t off_mid;
    uint32_t off_hi;
    uint32_t zero;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES] __attribute__((aligned(16)));

static struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr;

extern const void *const isr_stubs[32];

static void set_gate(int n, uint64_t handler) {
    idt[n].off_lo = handler & 0xffff;
    idt[n].selector = 0x08;
    idt[n].ist_type = GATE_INTR;
    idt[n].off_mid = (handler >> 16) & 0xffff;
    idt[n].off_hi = handler >> 32;
}

static void halt(void) {
    for (;;)
        __asm__ volatile("cli; hlt");
}

/* 错误的简称 */
static const char *const names[32] = {
    "#DE", "#DB", "NMI", "#BP", "#OF", "#BR", "#UD", "#NM", "#DF", "9",   "#TS",
    "#NP", "#SS", "#GP", "#PF", "15",  "#MF", "#AC", "#MC", "#XM", "#VE", "#CP",
    "22",  "23",  "24",  "25",  "26",  "27",  "#HV", "29",  "#VC", "#SX",
};

void x86_exception(const struct frame *f) {
    serial_puts("kernel: exception ");
    serial_puts(f->vector < 32 ? names[f->vector] : "?");
    serial_puts(" vector=");
    serial_puthex(f->vector);
    serial_puts(" error=");
    serial_puthex(f->error);
    serial_puts(" rip=");
    serial_puthex(f->rip);
    serial_putc('\n');
    halt();
}

void idt_init(void) {
    for (int i = 0; i < 32; i++)
        set_gate(i, (uint64_t)isr_stubs[i]);

    idtr.limit = sizeof(idt) - 1;
    idtr.base = (uint64_t)idt;
    __asm__ volatile("lidt %0" : : "m"(idtr));
}
