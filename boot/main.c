#include <stddef.h>

#include "efi.h"
#include "elf.h"
#include "mb2.h"

#define KERNEL_PATH u"\\kernel.elf"

/* 内核镜像上限 */
#define MAX_KERNEL_SIZE (16u << 20)

static EFI_BOOT_SERVICES *BS;
static EFI_HANDLE IH;

static inline void out8(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t in8(uint16_t port) {
    uint8_t v;
    __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static void putc_com1(char c) {
    while (!(in8(0x3f8 + 5) & 0x20))
        ;
    out8(0x3f8, (uint8_t)c);
}

static void puts_com1(const CHAR16 *s) {
    while (*s)
        putc_com1((char)(*s++));
}

static void die(const CHAR16 *msg) {
    puts_com1(u"boot: FAILED: ");
    puts_com1(msg);
    puts_com1(u"\r\n");

    for (;;)
        BS->Stall(1000000);
}

static void *read_file(CHAR16 *path, uint64_t *size_out) {
    EFI_LOADED_IMAGE_PROTOCOL *li;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfs;
    EFI_FILE_PROTOCOL *root, *file;
    void *buf;

    if (BS->HandleProtocol(IH, &EFI_LOADED_IMAGE_PROTOCOL_GUID, (void **)&li) !=
        EFI_SUCCESS)
        die(u"no loaded image protocol");
    if (BS->HandleProtocol(li->DeviceHandle,
                           &EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID,
                           (void **)&sfs) != EFI_SUCCESS)
        die(u"boot device is not a FAT volume");
    if (sfs->OpenVolume(sfs, &root) != EFI_SUCCESS)
        die(u"cannot open volume");
    if (root->Open(root, (void **)&file, path, EFI_FILE_MODE_READ, 0) !=
        EFI_SUCCESS)
        die(u"cannot open " KERNEL_PATH);

    if (BS->AllocatePool(EFI_LOADER_DATA, MAX_KERNEL_SIZE, &buf) != EFI_SUCCESS)
        die(u"cannot allocate for kernel image");

    uint8_t *p = buf;
    uint64_t total = 0;
    for (;;) {
        uint64_t chunk = MAX_KERNEL_SIZE - total;
        if (chunk == 0)
            die(u"kernel image too large");
        if (file->Read(file, &chunk, p) != EFI_SUCCESS)
            die(u"cannot read " KERNEL_PATH);
        if (chunk == 0)
            break;
        total += chunk;
        p += chunk;
    }

    file->Close(file);
    root->Close(root);
    *size_out = total;
    return buf;
}

/*
 * 按 p_paddr 放置 PT_LOAD 段
 */
static uint64_t load_kernel(const void *image) {
    const Elf64_Ehdr *eh = image;
    const Elf64_Phdr *ph = image + eh->e_phoff;

    if (eh->e_ident[0] != 0x7f || eh->e_ident[1] != 'E' ||
        eh->e_machine != 62 /* EM_X86_64 */ || eh->e_phnum == 0)
        die(u"kernel.elf is not an x86_64 ELF image");

    for (int i = 0; i < eh->e_phnum; i++) {
        const Elf64_Phdr *p = &ph[i];

        if (p->p_type != PT_LOAD)
            continue;

        uint64_t pages = EFI_SIZE_TO_PAGES(p->p_memsz);
        uint64_t addr = p->p_paddr;
        if (BS->AllocatePages(EFI_ALLOCATE_ADDRESS, EFI_LOADER_DATA, pages,
                              &addr) != EFI_SUCCESS)
            die(u"cannot place kernel segment (not 1:1 mapped?)");

        uint8_t *dst = (uint8_t *)p->p_paddr;
        for (uint64_t j = 0; j < p->p_filesz; j++)
            dst[j] = ((const uint8_t *)image)[p->p_offset + j];
        for (uint64_t j = p->p_filesz; j < p->p_memsz; j++)
            dst[j] = 0;
    }

    return eh->e_entry;
}

/* EFI 内存类型 */
static uint32_t mmap_type(uint32_t efi_type) {
    switch (efi_type) {
    case EFI_CONVENTIONAL:
        return MB2_MMAP_AVAILABLE;
    case EFI_ACPI_RECLAIM:
        return MB2_MMAP_ACPI;
    case EFI_ACPI_NVS:
        return MB2_MMAP_NVS;
    default:
        return MB2_MMAP_RESERVED;
    }
}

struct info_builder {
    uint8_t *base;
    uint64_t used;
};

static void *append_tag(struct info_builder *b, uint32_t type, uint32_t size) {
    uint64_t off =
        (b->used + MB2_TAG_ALIGN - 1) & ~(uint64_t)(MB2_TAG_ALIGN - 1);
    struct mb2_tag *t = (struct mb2_tag *)(b->base + off);

    t->type = type;
    t->size = size;
    b->used = off + size;
    return t;
}

static void add_mmap_tag(struct info_builder *b,
                         const EFI_MEMORY_DESCRIPTOR *map, uint64_t map_size,
                         uint64_t desc_size, uint64_t desc_version) {
    struct mb2_tag_mmap *tag;

    /* EFI 描述符 与 Multiboot2 条目 不一，因此需要转换 */
    tag = append_tag(b, MB2_TAG_MMAP,
                     sizeof(*tag) +
                         map_size / desc_size * sizeof(struct mb2_mmap_entry));
    tag->entry_size = sizeof(struct mb2_mmap_entry);
    tag->entry_version = (uint32_t)desc_version;

    struct mb2_mmap_entry *out = tag->entries;
    for (uint64_t off = 0; off < map_size; off += desc_size) {
        const EFI_MEMORY_DESCRIPTOR *d =
            (const EFI_MEMORY_DESCRIPTOR *)((const uint8_t *)map + off);

        out->addr = d->PhysicalStart;
        out->len = d->Pages * EFI_PAGE_SIZE;
        out->type = mmap_type(d->Type);
        out->reserved = 0;
        out++;
    }
}

static void add_framebuffer_tag(struct info_builder *b)
{
    EFI_HANDLE *handles;
    uint64_t n;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    struct mb2_tag_framebuffer *tag;

    if (BS->LocateHandleBuffer(EFI_LOCATE_BY_PROTOCOL,
                               &EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID, NULL,
                               &n, &handles) != EFI_SUCCESS)
        die(u"no GOP handle");

    for (uint64_t i = 0; i < n && gop == NULL; i++) {
        if (BS->HandleProtocol(handles[i], &EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID,
                               (void **)&gop) != EFI_SUCCESS)
            gop = NULL;
    }
    if (gop == NULL || gop->Mode == NULL)
        die(u"GOP unusable");

    tag = append_tag(b, MB2_TAG_FRAMEBUFFER, sizeof(*tag));
    tag->addr = gop->Mode->FrameBufferBase;
    tag->pitch = gop->Mode->Info->PixelsPerScanLine * 4;
    tag->width = gop->Mode->Info->HorizontalResolution;
    tag->height = gop->Mode->Info->VerticalResolution;
    tag->bpp = 32;
    tag->type = 1;
    tag->red_pos = 16;
    tag->red_mask = 8;
    tag->green_pos = 8;
    tag->green_mask = 8;
    tag->blue_pos = 0;
    tag->blue_mask = 8;
}

static void get_map(EFI_MEMORY_DESCRIPTOR **map_out, uint64_t *size_out,
                    uint64_t *key_out, uint64_t *desc_size_out,
                    uint32_t *desc_ver_out) {
    uint64_t size = EFI_PAGE_SIZE;
    EFI_MEMORY_DESCRIPTOR *map;

    for (;;) {
        if (BS->AllocatePages(EFI_ALLOCATE_ANY_PAGES, EFI_BOOT_SERVICES_DATA,
                              EFI_SIZE_TO_PAGES(size),
                              (EFI_PHYSICAL_ADDRESS *)&map) != EFI_SUCCESS)
            die(u"cannot allocate memory map buffer");

        uint64_t real_size = size;
        if (BS->GetMemoryMap(&real_size, map, key_out, desc_size_out,
                             desc_ver_out) == EFI_SUCCESS) {
            *map_out = map;
            *size_out = real_size;
            return;
        }
        size = real_size * 2;
    }
}

extern void boot_jump(uint64_t entry, uint64_t info, uint64_t stack);

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *st) {
    BS = st->BootServices;
    IH = image_handle;

    uint64_t map_size;
    EFI_MEMORY_DESCRIPTOR *map;
    uint64_t map_key, desc_size;
    uint32_t desc_version;

    get_map(&map, &map_size, &map_key, &desc_size, &desc_version);

    uint64_t info_size = EFI_PAGE_SIZE * 4;
    uint8_t *info;
    if (BS->AllocatePages(EFI_ALLOCATE_ANY_PAGES, EFI_LOADER_DATA,
                          EFI_SIZE_TO_PAGES(info_size),
                          (EFI_PHYSICAL_ADDRESS *)&info) != EFI_SUCCESS)
        die(u"cannot allocate boot info");

    struct info_builder b = {info, sizeof(struct mb2_info)};

    add_framebuffer_tag(&b);
    add_mmap_tag(&b, map, map_size, desc_size, desc_version);
    append_tag(&b, MB2_TAG_END, sizeof(struct mb2_tag));
    ((struct mb2_info *)info)->total_size = (uint32_t)b.used;

    uint64_t image_size;
    void *image = read_file(KERNEL_PATH, &image_size);
    uint64_t entry = load_kernel(image);
    BS->FreePool(image);

    uint64_t jump_stack;
    if (BS->AllocatePages(EFI_ALLOCATE_ANY_PAGES, EFI_LOADER_DATA, 1,
                          &jump_stack) != EFI_SUCCESS)
        die(u"cannot allocate transition stack");

    BS->SetWatchdogTimer(0, 0, 0, NULL);

    for (;;) {
        if (BS->ExitBootServices(IH, map_key) == EFI_SUCCESS)
            break;
        get_map(&map, &map_size, &map_key, &desc_size, &desc_version);
    }

    boot_jump(entry, (uint64_t)info, jump_stack + EFI_PAGE_SIZE);

    for (;;)
        ;
    return 0;
}
