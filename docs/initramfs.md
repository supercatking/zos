# Initramfs and Shell

M5 adds a small in-kernel initramfs and a user-mode shell.

## Files

The first initramfs contains:

- `/README`
- `/proc/status`

The kernel exposes files through `open`, `read`, and `close` syscalls.
Writable ramfs files can be created, renamed, removed, listed, read, and
overwritten while the kernel is running. They are intentionally volatile.

## Shell

The shell runs in U-mode and uses serial fd `0`/`1` as its terminal.

Commands:

- `help`
- `echo`
- `ls`
- `cat`
- `touch`
- `rm`
- `mkdir`
- `rmdir`
- `stat`
- `cp`
- `mv`
- `ps`
- `sleep`
- `kill`
- `uptime`
- `mem`
- `uname`
- `pwd`
- `cd`
- `env`
- `which`
- `history`
- `grep`
- `wc`
- `true`
- `false`
- `clear`
- `reboot`

The command parser is intentionally tiny, but it now tokenizes `argc`/`argv`
and dispatches through a builtin command table. Commands are still shell
builtins when they need shell-local state. `help`, `echo`, `cat`, `ls`,
`forktest`, `multiforktest`, `vmtest`, and `schedtest` are also available as
embedded `/bin/*` user programs and run through the `fork`/`exec`/`wait` path.

## Smoke Test Input

The QEMU smoke script can feed serial input:

```sh
QEMU_SMOKE_INPUT='help\nwhich echo\n/bin/echo hello\necho hello\n/bin/forktest\n/bin/multiforktest\n/bin/vmtest\n/bin/schedtest\ntouch a\nls /bin\nls\necho hello > a\ncat a\ncat /README\nps\npwd\nclear\nenv\nhistory\ngrep hello a\nwc a\ntrue\nfalse\ncd /\nreboot\n' \
QEMU_SMOKE_EXPECT='commands:;echo;hello;forktest: child saw 0;forktest: wait reaped child;multifork: wait reaped 3;multifork: ok;vmtest: isolation ok;schedtest: wait reaped 3;schedtest: ok;multiforktest;schedtest;a;ZOS README;pid: 1 ppid: 0 state: running name: sh;PATH=/bin;history;lines=1 words=1 bytes=6;user: halted cleanly' \
make test
```
