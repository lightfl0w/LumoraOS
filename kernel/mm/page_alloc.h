#ifndef KERNEL_MM_PAGE_ALLOC_H
#define KERNEL_MM_PAGE_ALLOC_H

#include <stdint.h>

struct mb2_info;

void page_alloc_init(const struct mb2_info *info);
void *page_alloc(void);
void page_free(void *page);
uint64_t page_total_count(void);
uint64_t page_free_count(void);

#endif
