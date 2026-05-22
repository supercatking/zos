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
| 8 | `create` | `a0=path` | `0` or `-1` |
| 9 | `list` | `a0=buf`, `a1=len` | bytes written |
| 10 | `unlink` | `a0=path` | `0` or `-1` |
| 11 | `stat` | `a0=path`, `a1=buf`, `a2=len` | bytes written or `-1` |
| 12 | `mkdir` | `a0=path` | `0` or `-1` |
| 13 | `rename` | `a0=old_path`, `a1=new_path` | `0` or `-1` |
| 14 | `uptime` | none | seconds since kernel timer init |
| 15 | `meminfo` | `a0=buf`, `a1=len` | bytes written or `-1` |
| 16 | `exec` | `a0=path`, `a1=arg string` | does not return on success, `-1` on failure |
| 17 | `fork` | none | `0` in child, child pid in parent, or `-1` |
| 18 | `wait` | none | child pid or `-1` |
| 19 | `getpid` | none | current pid |
| 20 | `procinfo` | `a0=buf`, `a1=len` | bytes written |

Fd `0` reads from the serial terminal. Fd `1` and `2` write to the serial
terminal. File descriptors from `3` upward refer to ramfs files and support
`read`, `write`, and `close`. M11 routes file syscalls through the VFS layer:
ramfs is mounted at `/`, `/dev/console` is a console device node, and `/proc/*`
nodes are generated dynamically.

## Current User Program

The user shell and `/bin/*` programs are linked into the kernel image and loaded
from the in-kernel ramfs. Each process owns its own Sv32 user page table, user
text pages, and user stack page. Kernel mappings are shared into each process
page table.

`fork` creates a child process with copied user pages and places it in the
runnable process set. Timer interrupts can switch between runnable user
processes. `wait` reaps zombie children when one is available; otherwise it
blocks the parent until a child exits. `exit` marks non-init processes zombie,
wakes a waiting parent, and lets `wait` reclaim the child slot.

The scheduler is intentionally simple: single hart, fixed process table, no
copy-on-write, and no priority policy.
