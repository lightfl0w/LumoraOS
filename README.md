# lumora-os

一个Hobby OS

## 依赖

- clang + lld（`lld-link`）
- Meson、Ninja
- mtools（`mformat`、`mmd`、`mcopy`）
- QEMU + OVMF 固件

## 构建与运行

```
meson setup build --cross-file cross/x86_64-elf.ini
ninja -C build        # 产出 kernel.elf、boot.efi、esp.img
ninja -C build run    # 在 QEMU 中引导 ESP 镜像
```
