# Initramfs and Shell

M5 adds a small in-kernel initramfs and a user-mode shell.

## Files

The first initramfs contains:

- `/README`
- `/proc/status`

The kernel exposes files through `open`, `read`, and `close` syscalls.

## Shell

The shell runs in U-mode and uses serial fd `0`/`1` as its terminal.

Commands:

- `help`
- `echo`
- `ls`
- `cat`
- `ps`
- `sleep`
- `kill`
- `reboot`

The command parser is intentionally tiny and currently dispatches by the first
character of the input line.

## Smoke Test Input

The QEMU smoke script can feed serial input:

```sh
QEMU_SMOKE_INPUT='help\nls\ncat\nps\nsleep\nkill\nreboot\n' \
QEMU_SMOKE_EXPECT='commands: help;ZOS README;pid  name;sleep done;kill: pid 1 noted;user: halted cleanly' \
make test
```
