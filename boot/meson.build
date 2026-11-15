efi_cflags = [
  '--target=x86_64-pc-windows-gnu',
  '-ffreestanding',
  '-fno-stack-protector',
  '-mno-red-zone',
  '-O2',
  '-g',
  '-Wall',
  '-Wextra',
  '-I' + meson.current_source_dir(),
  '-I' + meson.project_source_root() / 'include',
]

boot_c_o = custom_target('boot-main.o',
  build_by_default: true,
  input: ['main.c', 'efi.h', 'elf.h'],
  output: 'boot-main.o',
  command: ['clang'] + efi_cflags + ['-c', '@INPUT0@', '-o', '@OUTPUT@'],
)

boot_asm_o = custom_target('boot-jump.o',
  build_by_default: true,
  input: 'jump.S',
  output: 'boot-jump.o',
  command: ['clang'] + efi_cflags + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
)

boot_efi = custom_target('boot.efi',
  build_by_default: true,
  input: [boot_c_o, boot_asm_o],
  output: 'boot.efi',
  command: ['lld-link',
    '/subsystem:efi_application',
    '/entry:efi_main',
    '/base:0',
    '/machine:x64',
    '/dynamicbase:no',
    '/out:@OUTPUT@',
    '@INPUT0@', '@INPUT1@'],
)
