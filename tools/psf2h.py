#!/usr/bin/env python3
import gzip
import struct
import sys

PSF2_MAGIC = 0x864ab572


def load(path):
    raw = gzip.open(path, "rb").read() if path.endswith(".gz") else open(path, "rb").read()

    if struct.unpack_from("<I", raw)[0] == PSF2_MAGIC:
        hdr_size = struct.unpack_from("<I", raw, 8)[0]
        chars, _, height, width = struct.unpack_from("<4I", raw, 16)
    elif raw[:2] == b"\x36\x04":  # PSF1
        hdr_size, chars, height, width = 4, 256, raw[3], 8
    else:
        sys.exit("not a PSF font")

    step = (width + 7) // 8 * height
    glyphs = [raw[hdr_size + i * step:hdr_size + (i + 1) * step] for i in range(chars)]
    return glyphs, width, height


def main(path):
    glyphs, width, height = load(path)
    step = (width + 7) // 8 * height

    print("/* 由 tools/psf2h.py 从 %s 生成，勿手改 */" % path)
    print("#ifndef KERNEL_DRIVERS_FONT_H")
    print("#define KERNEL_DRIVERS_FONT_H")
    print("")
    print("#define FONT_W %d" % width)
    print("#define FONT_H %d" % height)
    print("")
    print("/* 每字形 %d 字节，按行；下标 = ASCII - 0x20 */" % step)
    print("static const unsigned char font_glyphs[0x5f][%d] = {" % step)
    for code in range(0x20, 0x7f):
        rows = ", ".join("0x%02x" % b for b in glyphs[code])
        name = "backslash" if code == 0x5c else chr(code)
        print("    {%s}, /* %s */" % (rows, name))
    print("};")
    print("")
    print("#endif")


if __name__ == "__main__":
    main(sys.argv[1])
