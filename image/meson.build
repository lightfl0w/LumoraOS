qemu = find_program('qemu-system-x86_64')
ovmf = get_option('ovmf')

esp = custom_target('esp.img',
  build_by_default: true,
  input: [boot_efi, kernel_elf],
  output: 'esp.img',
  command: ['sh', '-c',
    'mformat -i "$1" -F -C -T 131072 && ' +
    'mmd -i "$1" ::/EFI ::/EFI/BOOT && ' +
    'mcopy -i "$1" "$2" ::/EFI/BOOT/BOOTX64.EFI && ' +
    'mcopy -i "$1" "$3" ::/kernel.elf',
    'mkesp', '@OUTPUT@', '@INPUT0@', '@INPUT1@'],
)

run_target('run',
  command: [qemu,
    '-bios', ovmf,
    '-drive', 'format=raw,file=' + esp.full_path(),
    '-serial', 'stdio',
    '-no-reboot', '-no-shutdown'],
)
