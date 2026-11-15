kernel_elf = executable('kernel.elf',
  'arch/x86_64/boot.S',
  'core/main.c',
  'drivers/serial.c',
  c_args: common_cflags,
  link_args: common_cflags + [
    '-nostdlib',
    '-static',
    '-Wl,-T,' + meson.current_source_dir() / 'arch/x86_64/linker.ld',
    '-Wl,--build-id=none',
    '-Wl,-z,max-page-size=4096',
  ],
)
