#!/usr/bin/env python3
import struct
import sys
from pathlib import Path

SECTOR = 512
IMAGE_SIZE = 1024 * 1024

MAGIC = 0x5A4F5346
VERSION = 1
MAX_INODES = 16
DIRECT_BLOCKS = 4

SUPER_SECTOR = 1
INODE_SECTOR = 2
DIRENT_SECTOR = 4
DATA_START_SECTOR = 8

INODE_FREE = 0
INODE_FILE = 1
INODE_DIR = 2

README = b"ZOS DISK README\nThis file is loaded from the simple disk filesystem.\n"
BIG = (b"0123456789abcdef " * 70) + b"\n"


def write_at(image: bytearray, sector: int, data: bytes) -> None:
    start = sector * SECTOR
    image[start:start + len(data)] = data


def inode(inode_type: int, size: int, blocks: list[int]) -> bytes:
    padded = blocks[:DIRECT_BLOCKS] + [0] * (DIRECT_BLOCKS - len(blocks))
    return struct.pack("<II4I", inode_type, size, *padded)


def dirent(ino: int, name: str) -> bytes:
    raw = name.encode("ascii")
    if len(raw) >= 28:
        raise ValueError("dirent name too long")
    return struct.pack("<I28s", ino, raw + b"\0" * (28 - len(raw)))


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: mkfs_zos.py OUTPUT", file=sys.stderr)
        return 2

    out = Path(sys.argv[1])
    out.parent.mkdir(parents=True, exist_ok=True)
    image = bytearray(IMAGE_SIZE)

    superblock = struct.pack(
        "<6I",
        MAGIC,
        VERSION,
        IMAGE_SIZE // SECTOR,
        MAX_INODES,
        DATA_START_SECTOR,
        1,
    )
    write_at(image, SUPER_SECTOR, superblock)

    inodes = bytearray(SECTOR * 2)
    inodes[0:24] = inode(INODE_DIR, 64, [DIRENT_SECTOR])
    inodes[24:48] = inode(INODE_FILE, len(README), [DATA_START_SECTOR])
    big_blocks = [DATA_START_SECTOR + DIRECT_BLOCKS * 2 + i for i in range(3)]
    inodes[48:72] = inode(INODE_FILE, len(BIG), big_blocks)
    write_at(image, INODE_SECTOR, inodes)

    dirents = bytearray(SECTOR)
    dirents[0:32] = dirent(2, "README")
    dirents[32:64] = dirent(3, "BIG")
    write_at(image, DIRENT_SECTOR, dirents)
    write_at(image, DATA_START_SECTOR, README)
    for i, block in enumerate(big_blocks):
        write_at(image, block, BIG[i * SECTOR:(i + 1) * SECTOR])

    out.write_bytes(image)
    print(f"created {out} ({len(image)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
