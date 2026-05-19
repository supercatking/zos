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

Only fd `1` and `2` are accepted by `write` in M4.

## Current User Program

The first user program is linked into the kernel image, copied into a user text
page, mapped with `PTE_U`, and entered with `sret`.
