# System Calls

M4 introduces a tiny ZOS syscall ABI for the first user-mode program.

## ABI

- `a7`: syscall number
- `a0`..`a5`: arguments
- `a0`: return value
- `ecall`: enter the kernel from U-mode

The trap handler advances `sepc` by 4 before returning from handled syscalls.

## Current Calls

| Number | Name | Arguments | Result |
| --- | --- | --- | --- |
| 1 | `write` | `a0=fd`, `a1=buf`, `a2=len` | bytes written or `-1` |
| 2 | `exit` | `a0=status` | does not return |
| 3 | `read` | `a0=fd`, `a1=buf`, `a2=len` | bytes read or `-1` |
| 4 | `open` | `a0=path` | fd or `-1` |
| 5 | `close` | `a0=fd` | `0` or `-1` |
| 6 | `sleep` | `a0=ticks` | `0` |
| 7 | `kill` | `a0=pid` | `0` or `-1` |

Only fd `1` and `2` are accepted by `write` in M4.
Fd `0` reads from the serial terminal. File descriptors from `3` upward refer
to initramfs files.

## Current User Program

The first user shell is linked into the kernel image, copied into a user page,
mapped with `PTE_U`, and entered with `sret`.
