# Toolchain Setup

This project targets a RISC-V32 kernel running on QEMU's `virt` machine. WSL
Ubuntu is the recommended development environment on Windows.

## Install Dependencies on WSL/Ubuntu

Install the full local validation environment:

```sh
sudo apt-get update && sudo apt-get install -y qemu-system-misc opensbi gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf gdb-multiarch
```

Optional host-side helpers are useful but not required for the first kernel:

```sh
sudo apt-get install -y build-essential clang lld llvm make git ripgrep
```

`sudo` may prompt for your Linux password. If the package names differ on your
Ubuntu release, search with:

```sh
apt search riscv qemu-system
```

## Expected Tools

Check that the expected commands resolve inside WSL:

```sh
which make
which riscv64-unknown-elf-gcc
which riscv64-unknown-elf-objcopy
which qemu-system-riscv32
test -f /usr/lib/riscv32-linux-gnu/opensbi/generic/fw_dynamic.bin
which gdb-multiarch
```

The Makefile can currently fall back to Ubuntu's Linux-targeted RISC-V compiler
for the M0/M1 freestanding build:

```sh
which riscv64-linux-gnu-gcc
```

For a 32-bit teaching kernel, Clang can normally target RISC-V32 directly with a
target triple such as:

```sh
clang --target=riscv32-unknown-elf -march=rv32im -mabi=ilp32
```

The GNU package names often say `riscv64`, but the bare-metal tools can usually
emit 32-bit RISC-V when passed `-march=rv32...` and `-mabi=ilp32`.

## QEMU Command Shape

The first runnable milestone should use a command like:

```sh
qemu-system-riscv32 \
  -machine virt \
  -nographic \
  -bios /usr/lib/riscv32-linux-gnu/opensbi/generic/fw_dynamic.bin \
  -kernel build/kernel.elf
```

Useful debug shape:

```sh
qemu-system-riscv32 \
  -machine virt \
  -nographic \
  -bios /usr/lib/riscv32-linux-gnu/opensbi/generic/fw_dynamic.bin \
  -kernel build/kernel.elf \
  -S -s
```

Then attach GDB from another WSL shell:

```sh
gdb-multiarch build/kernel.elf
(gdb) target remote :1234
```

## Troubleshooting

### `qemu-system-riscv32: command not found`

Install QEMU system emulators:

```sh
sudo apt install -y qemu-system-misc
```

Then verify:

```sh
which qemu-system-riscv32
qemu-system-riscv32 --version
```

### `Unable to find the RISC-V BIOS "opensbi-riscv32-generic-fw_dynamic.bin"`

Install OpenSBI firmware:

```sh
sudo apt-get install -y opensbi
```

### `sudo` asks for a password

This is normal in WSL/Ubuntu. Use the Linux user password created for the WSL
distribution. If you do not know it, reset it from an administrator Windows
shell or ask whoever created the WSL environment.

### `rg` fails with `Permission denied` in WSL

The shell may be finding a Windows `rg.exe` through the WSL PATH before a Linux
binary. Check what is being used:

```sh
which rg
type -a rg
```

If the path points into `/mnt/c/...`, install the Linux package and ensure it is
found first:

```sh
sudo apt install -y ripgrep
hash -r
which rg
```

As a temporary workaround, use `find`, `grep`, or the absolute Linux path for
`rg`.

### Kernel boots with no output

- Confirm the kernel was linked for the QEMU `virt` memory map.
- Confirm the UART base address and initialization match the platform.
- Run with `-nographic` so serial output appears in the terminal.
- Add an early infinite loop after the first UART write to distinguish boot
  failure from later kernel failure.

### GDB cannot connect

Start QEMU with `-S -s`, then connect to TCP port 1234:

```sh
gdb-multiarch build/kernel.elf
(gdb) target remote :1234
```

If the port is busy, stop old QEMU processes or choose an explicit `-gdb`
endpoint.
