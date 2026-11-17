#ifndef KERNEL_SCHED_THREAD_H
#define KERNEL_SCHED_THREAD_H

#include <stdint.h>

struct thread;

void sched_init(void);
struct thread *thread_create(void (*fn)(void *), void *arg);
void thread_yield(void);
void sched_tick(void);

#endif
