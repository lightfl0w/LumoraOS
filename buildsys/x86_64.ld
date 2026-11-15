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
        *(.text .text.*)
    }
    __lma = ALIGN(__lma + SIZEOF(.text), 4096);
    . = ALIGN(4096);

    .rodata : AT(__lma) {
        *(.rodata .rodata.*)
    }
    __lma = ALIGN(__lma + SIZEOF(.rodata), 4096);
    . = ALIGN(4096);

    .data : AT(__lma) {
        *(.data .data.*)
    }
    __lma = ALIGN(__lma + SIZEOF(.data), 4096);
    . = ALIGN(4096);

    .bss : AT(__lma) {
        *(COMMON)
        *(.bss .bss.*)
    }

    __kernel_phys_start = __boot_base;
    __kernel_phys_end = __lma + SIZEOF(.bss);

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note*)
        *(.comment)
    }
}
