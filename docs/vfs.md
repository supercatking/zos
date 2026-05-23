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

Fd `0` currently reads from the serial terminal directly in the syscall layer.
Fd `1` and `2` write to the serial terminal directly. Other descriptors are VFS
descriptors. M15 should move toward standard fd semantics and add pipes.

## Limits

The VFS does not yet model inode lifetimes, fd flags, `dup`, pipes, `select`,
`poll`, permissions, ownership, mount flags, or full path normalization.
