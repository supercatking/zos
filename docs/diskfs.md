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
```

## Limits

The current disk filesystem supports root-level files and one-block writes. It
does not yet support nested directories, multi-block files, unlink/rename on
disk, timestamps, permissions, fsck, or crash consistency.
