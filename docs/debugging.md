# Debugging ZOS

These notes cover the early QEMU/OpenSBI kernel used through M2.

## Serial Smoke Test

Run the default smoke test:

```sh
make test
```

Require extra output patterns:

```sh
QEMU_SMOKE_EXPECT='trap: initialized;timer: initialized;thread: alpha;timer: tick' make test
```

The smoke script captures QEMU output, checks the expected serial patterns, and
terminates QEMU with `timeout`.

## Manual QEMU Run

```sh
make build
qemu-system-riscv32 \
  -machine virt \
  -bios /usr/lib/riscv32-linux-gnu/opensbi/generic/fw_dynamic.bin \
  -nographic \
  -kernel build/kernel.elf
```

Exit QEMU with `Ctrl+A`, then `X`.

## GDB

Start QEMU halted:

```sh
qemu-system-riscv32 \
  -machine virt \
  -bios /usr/lib/riscv32-linux-gnu/opensbi/generic/fw_dynamic.bin \
  -nographic \
  -kernel build/kernel.elf \
  -S -s
```

Attach from another WSL shell:

```sh
gdb-multiarch build/kernel.elf
(gdb) target remote :1234
(gdb) break kernel_main
(gdb) continue
```

Useful symbols for M2 are `trap_entry`, `trap_handler`, `timer_handle_interrupt`,
`thread_switch`, and `thread_start_scheduler`.

## Trap Notes

- `scause` bit 31 means interrupt on RV32.
- Supervisor timer interrupt code is 5.
- Unknown traps print `scause`, `stval`, and `sepc`, then panic.
- Timer interrupts are intentionally quiet except for periodic tick output.
