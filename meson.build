project('lumora-os', 'c',
  version: '0.0.1',
  default_options: ['c_std=c11', 'warning_level=2'])

common_cflags = [
  '-ffreestanding',
  '-fno-pic',
  '-fno-pie',
  '-fno-stack-protector',
  '-mno-red-zone',
  '-fno-omit-frame-pointer',
]

subdir('kernel')
subdir('boot')
subdir('image')
