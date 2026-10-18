#!/usr/bin/env python3
"""Fills in the image header (length and CRC-32) of a built Yarrow .bin in place."""
import struct
import sys
import zlib

HEADER_OFFSET = 0x200
MAGIC = 0x57524159
MAX_SIZE = 0x18000

path = sys.argv[1]
if len(sys.argv) > 2:
    MAX_SIZE = int(sys.argv[2], 0)
image = bytearray(open(path, "rb").read())

while len(image) % 4:
    image.append(0)

magic, _, _, version = struct.unpack_from("<4I", image, HEADER_OFFSET)
if magic != MAGIC:
    sys.exit(f"{path}: image header magic not found at 0x{HEADER_OFFSET:x}")
if len(image) > MAX_SIZE:
    sys.exit(f"{path}: image is {len(image)} bytes, limit is {MAX_SIZE}")

struct.pack_into("<I", image, HEADER_OFFSET + 4, len(image))
struct.pack_into("<I", image, HEADER_OFFSET + 8, 0)
crc = zlib.crc32(bytes(image))
struct.pack_into("<I", image, HEADER_OFFSET + 8, crc)

open(path, "wb").write(image)
print(f"{path}: {len(image)} bytes, header crc32 0x{crc:08x}")
