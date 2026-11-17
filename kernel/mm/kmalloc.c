#include <stddef.h>
#include <stdint.h>

#include "kmalloc.h"
#include "addr.h"
#include "page_alloc.h"

#define HDR_SIZE 16u
#define KMIN 16u
#define KMAX 2048u
#define NCLASS 8u /* 16 * 2^0..7 */

static void *freelist[NCLASS];

static void push(uint32_t cls, void *obj) {
    *(void **)obj = freelist[cls];
    freelist[cls] = obj;
}

static void *pop(uint32_t cls) {
    void *obj = freelist[cls];

    if (obj)
        freelist[cls] = *(void **)obj;
    return obj;
}

static uint32_t class_of(uint32_t block) {
    return 31 - __builtin_clz(block / KMIN);
}

static void *refill(uint32_t cls) {
    char *page = phys_to_virt((uint64_t)page_alloc());

    if (page == NULL)
        return NULL;

    uint32_t size = KMIN << cls;

    for (uint32_t off = 0; off < PAGE_SIZE; off += size)
        push(cls, page + off);

    return pop(cls);
}

void *kmalloc(uint64_t size) {
    if (size == 0 || size > KMAX)
        return NULL;

    uint32_t block = ((uint32_t)size + HDR_SIZE + 15u) & ~15u;
    uint32_t cls = class_of(block);
    void *obj = freelist[cls];

    if (obj == NULL)
        obj = refill(cls);
    if (obj == NULL)
        return NULL;

    freelist[cls] = *(void **)obj;
    *(uint32_t *)obj = block;
    return (char *)obj + HDR_SIZE;
}

void kfree(void *ptr) {
    if (ptr == NULL)
        return;

    char *obj = (char *)ptr - HDR_SIZE;
    push(class_of(*(uint32_t *)obj), obj);
}
