OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

KERNEL_HIGH = 0xffffffff80000000;

SECTIONS
{
    . = 1M;
    __boot_base = .;

    .boot.text : {
        KEEP(*(.text.boot))
    }

    .boot.data : {
        *(.boot.data)
    }

    __boot_end = ALIGN(4096);

    . = ALIGN(4096);
    __lma = ALIGN(__boot_end, 4096);

    . = KERNEL_HIGH + __lma;

    .text : AT(__lma) {
        __text_start = .;
        *(.text .text.*)
        __text_end = .;
    }
    __lma = ALIGN(__lma + SIZEOF(.text), 4096);
    . = ALIGN(4096);

    .rodata : AT(__lma) {
        __rodata_start = .;
        *(.rodata .rodata.*)
        __rodata_end = .;
    }
    __lma = ALIGN(__lma + SIZEOF(.rodata), 4096);
    . = ALIGN(4096);

    .data : AT(__lma) {
        __data_start = .;
        *(.data .data.*)
        __data_end = .;
    }
    __lma = ALIGN(__lma + SIZEOF(.data), 4096);
    . = ALIGN(4096);

    .bss : AT(__lma) {
        __bss_start = .;
        *(COMMON)
        *(.bss .bss.*)
        __bss_end = .;
    }

    __kernel_phys_start = __boot_base;
    __kernel_phys_end = __lma + SIZEOF(.bss);

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note*)
        *(.comment)
    }
}
