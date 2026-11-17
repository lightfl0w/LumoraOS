#ifndef KERNEL_MM_ADDR_H
#define KERNEL_MM_ADDR_H

#include <stdint.h>

#define USER_TOP 0x0000800000000000ull
#define PHYSMAP_BASE 0xffff888000000000ull
#define PHYSMAP_SIZE 0x0000400000000000ull /* 512GiB */
#define KERNEL_BASE 0xffffffff80000000ull

static inline void *phys_to_virt(uint64_t pa) {
    return (void *)(PHYSMAP_BASE + pa);
}

static inline uint64_t virt_to_phys(const void *va) {
    return (uint64_t)va - PHYSMAP_BASE;
}

static inline uint64_t kernel_virt_to_phys(const void *va) {
    return (uint64_t)va - KERNEL_BASE;
}

#endif
