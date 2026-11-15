#ifndef KERNEL_MM_KMALLOC_H
#define KERNEL_MM_KMALLOC_H

#define KMALLOC_MAX 2048ull

#include <stdint.h>

void *kmalloc(uint64_t size);
void kfree(void *ptr);

#endif
