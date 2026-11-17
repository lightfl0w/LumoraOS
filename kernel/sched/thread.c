#include <stddef.h>
#include <stdint.h>

#include "../drivers/console.h"
#include "../mm/kmalloc.h"
#include "../mm/page_alloc.h"
#include "thread.h"

#define QUANTUM_TICKS 1
#define STACK_SIZE PAGE_SIZE

#define THREAD_READY 0
#define THREAD_RUNNING 1
#define THREAD_DEAD 2

struct thread {
    uint64_t rsp;
    uint32_t id;
    uint32_t state;
    struct thread *next;
    uint8_t *stack;
};

static struct thread *current;
static struct thread *ring;
static uint32_t next_id;
static uint32_t quantum;

static struct thread boot_thread;

extern void context_switch(uint64_t *old_rsp, uint64_t new_rsp);
extern void thread_entry_stub(void);
void thread_exit(void);

static void unlink(struct thread *t) {
    if (t->next == t) {
        ring = NULL;
        return;
    }

    struct thread *p = t->next;
    while (p->next != t)
        p = p->next;

    p->next = t->next;
    if (ring == t)
        ring = t->next;
}

static void schedule(void) {
    struct thread *next = current->next;

    if (next == current)
        return;

    struct thread *prev = current;
    prev->state = THREAD_READY;
    next->state = THREAD_RUNNING;
    current = next;
    context_switch(&prev->rsp, next->rsp);
}

struct thread *thread_create(void (*fn)(void *), void *arg) {
    struct thread *t = kmalloc(sizeof(*t));
    uint8_t *stack = page_alloc();

    if (t == NULL || stack == NULL) {
        kfree(t);
        return NULL;
    }

    uint64_t *sp = (uint64_t *)(stack + STACK_SIZE);

    *--sp = (uint64_t)thread_entry_stub;
    *--sp = 0;
    *--sp = 0;
    *--sp = (uint64_t)arg;
    *--sp = (uint64_t)fn;
    *--sp = 0;
    *--sp = 0;

    t->rsp = (uint64_t)sp;
    t->id = next_id++;
    t->state = THREAD_READY;
    t->stack = stack;
    t->next = current->next;
    current->next = t;
    return t;
}

void sched_init(void) {
    boot_thread.id = next_id++;
    boot_thread.state = THREAD_RUNNING;
    boot_thread.stack = NULL;
    boot_thread.next = &boot_thread;

    current = &boot_thread;
    ring = &boot_thread;
    quantum = 0;
}

void thread_yield(void) {
    uint64_t flags;

    __asm__ volatile("pushfq; popq %0; cli" : "=r"(flags) : : "memory");
    schedule();
    if (flags & 0x200)
        __asm__ volatile("sti");
}

void sched_tick(void) {
    if (current == NULL)
        return;
    if (++quantum < QUANTUM_TICKS)
        return;

    quantum = 0;
    schedule();
}

void thread_exit(void) {
    __asm__ volatile("cli");

    struct thread *dead = current;
    dead->state = THREAD_DEAD;
    unlink(dead);

    if (ring == NULL) {
        console_puts("kernel: no runnable thread\n");
        for (;;)
            __asm__ volatile("cli; hlt");
    }

    struct thread *next = ring;
    next->state = THREAD_RUNNING;
    current = next;
    context_switch(&dead->rsp, next->rsp);

    for (;;)
        __asm__ volatile("cli; hlt");
}
