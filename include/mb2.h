#ifndef MB2_H
#define MB2_H

#include <stdint.h>

#define MB2_BOOTLOADER_MAGIC 0x36d76289u

#define MB2_TAG_ALIGN 8

#define MB2_TAG_END 0
#define MB2_TAG_MMAP 6
#define MB2_TAG_FRAMEBUFFER 8

struct mb2_info {
    uint32_t total_size;
    uint32_t reserved;
};

struct mb2_tag {
    uint32_t type;
    uint32_t size;
};

struct mb2_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t reserved;
};

#define MB2_MMAP_AVAILABLE 1
#define MB2_MMAP_RESERVED 2
#define MB2_MMAP_ACPI 3
#define MB2_MMAP_NVS 4
#define MB2_MMAP_BAD 5

struct mb2_tag_mmap {
    struct mb2_tag tag;
    uint32_t entry_size;
    uint32_t entry_version;
    struct mb2_mmap_entry entries[];
};

struct mb2_tag_framebuffer {
    struct mb2_tag tag;
    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    uint8_t type;
    uint8_t red_pos;
    uint8_t red_mask;
    uint8_t green_pos;
    uint8_t green_mask;
    uint8_t blue_pos;
    uint8_t blue_mask;
};

#endif
