# VFS

ZOS routes file syscalls through a small VFS layer. The current interface covers
`open`, `read`, `write`, `close`, `create`, `list`, `unlink`, `mkdir`,
`rename`, and `stat`.

## Mounts

- `/`: volatile ramfs, including embedded `/bin/*` ELF programs and `/README`.
- `/proc`: dynamic procfs nodes such as `/proc/status`, `/proc/meminfo`,
  `/proc/uptime`, and `/proc/<pid>`.
- `/dev/console`: console device node; writes go to UART and reads currently
  return EOF through the VFS path.
- `/disk`: simple persistent filesystem, mounted when a virtio-blk disk image
  is attached.

## File Descriptors

Each process owns a small fd table. Fd `0` starts as console input, fd `1` and
`2` start as console output, and `open` installs VFS handles into free fd slots
from `3` upward. `dup2` lets the shell redirect standard streams for child
processes. M15 should add pipes next.

## Limits

The VFS does not yet model inode lifetimes, fd flags, `dup`, pipes, `select`,
`poll`, permissions, ownership, mount flags, or full path normalization.
