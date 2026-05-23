# Simple Disk Filesystem

The ZOS disk filesystem is a teaching format used by the QEMU virtio-blk smoke
tests. It is intentionally small and not compatible with ext2 or Linux
filesystems.

## Layout

- Sector 0: reserved for external boot/test data.
- Sector 1: superblock.
- Sectors 2-3: fixed inode table.
- Sector 4: root directory entries.
- Sector 8 onward: file data blocks.

The host tool `scripts/mkfs_zos.py` creates `build/zos.img` with a root
`/README` file.

## Usage

```sh
make clean build disk-image
qemu-system-riscv32 \
  -machine virt \
  -nographic \
  -bios /usr/lib/riscv32-linux-gnu/opensbi/generic/fw_dynamic.bin \
  -kernel build/kernel.elf \
  -drive file=build/zos.img,format=raw,if=none,id=hd0 \
  -device virtio-blk-device,drive=hd0
```

Inside ZOS:

```sh
ls /disk
cat /disk/README
echo hi > /disk/a
cat /disk/a
mkdir /disk/tmp
echo hi > /disk/tmp/a
ls /disk/tmp
mv /disk/tmp/a /disk/tmp/b
rm /disk/tmp/b
rmdir /disk/tmp
```

`make disk-regression` runs the same workflow headlessly through QEMU with a
virtio-blk disk attached.

## Limits

The current disk filesystem supports a small fixed inode table, one-block files,
root-level directories, files inside those directories, unlink, and rename.
Directories are encoded in the fixed root dirent table, so this is still a
teaching format rather than a general filesystem.

It does not yet support arbitrary-depth directories, directory rename with child
path rewriting, multi-block files, timestamps, permissions, fsck, or crash
consistency.
