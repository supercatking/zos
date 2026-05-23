# ZOS

ZOS is a small RISC-V32 teaching operating system that runs on QEMU's `virt`
machine through OpenSBI. The project goal is to grow a readable kernel into a
Linux-like OS with user processes, system calls, a shell, VFS, and block-backed
storage.

The implementation is intentionally modest: clear subsystem boundaries,
repeatable QEMU validation, and small milestone commits.

## Current Status

ZOS has progressed through M14:

- RISC-V32 S-mode kernel boot through OpenSBI.
- UART console, traps, timer interrupts, physical pages, and Sv32 paging.
- User-mode ELF programs with per-process page tables and kernel trap stacks.
- Process syscalls: `fork`, `exec`, `wait`, `exit`, `getpid`.
- Timer-driven round-robin scheduling for runnable user processes.
- VFS with ramfs mounted at `/`, procfs mounted at `/proc`, and console at
  `/dev/console`.
- Virtio-blk probing, a small block cache, and a simple persistent disk
  filesystem mounted at `/disk` when a disk image is attached.
- User shell plus embedded `/bin/*` programs including `echo`, `cat`, `ls`,
  `help`, `pwd`, `stat`, `grep`, `wc`, `true`, `false`, and regression tests.
- GitHub Actions CI for build, QEMU regression, disk image creation, and
  virtio-backed disk smoke testing.

The next development line is M15: standard fd semantics, redirection, and pipes.

## Prerequisites

Use WSL/Ubuntu or a native Ubuntu environment.

```sh
sudo apt-get update && sudo apt-get install -y \
  qemu-system-misc \
  opensbi \
  gcc-riscv64-unknown-elf \
  binutils-riscv64-unknown-elf \
  gdb-multiarch \
  make \
  python3
```

See [docs/toolchain.md](docs/toolchain.md) for troubleshooting.

## Build and Run

```sh
make clean build
make run
```

Manual QEMU command:

```sh
qemu-system-riscv32 \
  -machine virt \
  -nographic \
  -bios /usr/lib/riscv32-linux-gnu/opensbi/generic/fw_dynamic.bin \
  -kernel build/kernel.elf
```

Run with a persistent disk image:

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

## Test

```sh
make test
make regression
make disk-image
```

`make regression` drives the shell through user-program execution, process
tests, VFS/procfs checks, file creation, redirection, and clean shutdown.

## Documentation

- [docs/roadmap.md](docs/roadmap.md): milestone status and next steps.
- [docs/syscalls.md](docs/syscalls.md): syscall ABI and current syscall table.
- [docs/memory.md](docs/memory.md): physical memory, Sv32, and process address
  spaces.
- [docs/process.md](docs/process.md): process and scheduler model.
- [docs/vfs.md](docs/vfs.md): VFS, ramfs, procfs, console, and disk mounts.
- [docs/diskfs.md](docs/diskfs.md): simple on-disk filesystem format.
- [docs/initramfs.md](docs/initramfs.md): shell, ramfs, and smoke test notes.
- [docs/debugging.md](docs/debugging.md): QEMU/GDB workflow.

## Current Limits

ZOS is not Linux ABI compatible. It is single-hart, has a small fixed process
table, simple file descriptors, no pipes yet, no real TTY line discipline, no
signals, no demand paging, no networking, and a deliberately simple disk
filesystem.
