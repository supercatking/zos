# ZOS

ZOS is a small RISC-V32 teaching operating system. The goal is to grow it in
clear milestones from a bootable kernel skeleton into a tiny multi-tasking OS
with user programs, system calls, and a simple file system.

The project is intentionally modest: readable code, repeatable builds, and
documentation that explains why each subsystem exists.

## Current Status

ZOS is at the bootstrap stage.

- M0 project layout, build targets, and toolchain notes are present.
- M1 has a minimal RISC-V32 supervisor-mode kernel entry and UART console.
- The first runnable target is a RISC-V32 `virt` machine booting through
  OpenSBI under QEMU and printing early console output.

See [docs/roadmap.md](docs/roadmap.md) for the milestone plan.

## Prerequisites

Use WSL/Ubuntu or a native Ubuntu environment. The current expected tools are:

- `make`
- `riscv64-unknown-elf-gcc` or `riscv64-linux-gnu-gcc`
- matching GNU binutils for RISC-V
- `qemu-system-riscv32`
- `gdb-multiarch` or `riscv64-unknown-elf-gdb`

Some machines will not have the RISC-V QEMU and cross-toolchain packages
installed yet. Install them with `apt` as described in
[docs/toolchain.md](docs/toolchain.md). `sudo` may prompt for your Linux
password.

## Build

Build the kernel:

```sh
make build
```

Common targets:

```sh
make clean
make run
make test
```

## Run

The expected QEMU command shape for the first kernel image is:

```sh
qemu-system-riscv32 \
  -machine virt \
  -nographic \
  -bios /usr/lib/riscv32-linux-gnu/opensbi/generic/fw_dynamic.bin \
  -kernel build/kernel.elf
```

When `make run` exists, prefer:

```sh
make run
```

## Test

The intended test entrypoint is:

```sh
make test
```

Early tests may be smoke tests that build the kernel, boot it in QEMU, and check
for expected serial output.

## Notes

- Keep the teaching path simple before adding optimizations.
- Prefer small, reviewable milestone changes.
- Do not assume host tools are installed; document missing dependencies and the
  install command when a tool is required.
