# ZOS Roadmap

This roadmap tracks the agreed RISC-V32 teaching OS plan. Milestones are small
enough to verify independently and build toward a readable kernel.

## M0: Project Bootstrap

- Establish repository layout for kernel, user, include, scripts, and docs.
- Define the RISC-V32 target, linker layout, and QEMU `virt` run path.
- Document host dependencies and troubleshooting.
- Add minimal build targets for compile, clean, run, and smoke test when the
  kernel skeleton is ready.

Exit criteria: a new contributor can install tools, run the documented commands,
and understand the next milestone.

## M1: Boot and Console

- Add reset entry, linker script, stack setup, and C kernel handoff.
- Boot on `qemu-system-riscv32 -machine virt -bios /usr/lib/riscv32-linux-gnu/opensbi/generic/fw_dynamic.bin -nographic`.
- Initialize UART output for `printk`-style logging.
- Add panic and halt paths with clear serial output.

Exit criteria: QEMU boots the kernel and prints a deterministic banner.

## M2: Trap and Interrupt Foundation

- Set up supervisor-mode trap entry and register save/restore frame.
- Decode exceptions and interrupts enough for useful diagnostics.
- Configure timer interrupts on the QEMU `virt` platform.
- Add cooperative kernel threads with explicit context switching.
- Add smoke-testable output for trap initialization, timer ticks, and thread
  switching.

Exit criteria: QEMU prints `trap: initialized`, `timer: initialized`, alternating
thread iterations, and periodic timer ticks without corrupting kernel state.

## M3: Memory Management

- Define physical memory ranges for the QEMU `virt` machine.
- Add a page allocator for fixed-size pages.
- Introduce Sv32 kernel paging with an identity map for UART and RAM.
- Document memory layout and ownership rules.

Exit criteria: QEMU prints the PMM range, PMM self-test pass, kernel page table
readiness, and paging enabled while the M2 trap/timer/thread smoke test still
passes.

## M4: User Mode and System Calls

- Add user address space setup and transition from kernel to user mode.
- Load simple user programs from linked-in images or an initramfs.
- Implement the first syscall ABI for `write` and `exit`; keep `read`, `wait`,
  `fork`, `exec`, `open`, and `close` for M5/M6 expansion.
- Validate user pointers before the kernel touches user memory.

Exit criteria: QEMU enters U-mode, a user program prints through `write`, and
the kernel reports a clean `exit` status.

## M5: Initramfs and User Shell

- Add a tiny read-only initramfs image with `/README`.
- Provide file-descriptor backed `open`, `read`, and `close`, plus terminal
  `read`.
- Build a user-mode shell using the serial console as its terminal.
- Add first commands: `help`, `echo`, `cat`, `ls`, `ps`, and `reboot`.

Exit criteria: QEMU boots into a user-facing shell that can run simple commands.

## M6: Linux-like Extensions

- Extend syscall surface with `sleep`, `kill`, and clearer exit status output.
- Expose initramfs plus a proc-like `/proc/status` node.
- Prepare virtio-blk as the next storage backend without blocking the shell.
- Add regression tests for multi-program shell workflows.

Exit criteria: `make regression` drives the user shell through `help`, `ls`,
`cat`, `ps`, `sleep`, `kill`, and `reboot`.

## M7: User Programs and Process Syscalls

- Build multiple embedded `/bin/*` user programs.
- Add `exec`, `fork`, `wait`, and `getpid` syscall coverage.
- Route shell external commands through `fork`/`exec`/`wait`.
- Keep shell-local commands as builtins.

Exit criteria: QEMU regression runs `/bin/echo`, shell `echo`, `cat`, `ls /bin`,
and `ps`, then returns to the same shell.

## M8: Real Process Foundations

- Move user traps from the user stack onto a kernel trap stack.
- Give each process its own Sv32 page table, user text pages, and user stack.
- Add process-table-backed `procinfo` and user regression programs for
  `fork`/`wait` and address-space isolation.

Exit criteria: `make regression` runs `/bin/forktest` and `/bin/vmtest`, proves
parent/child memory isolation, and still completes all shell workflows.

## M9: Process Scheduler

- Add process states for runnable, running, blocked, sleeping, and zombie tasks.
- Put forked children into the runnable set instead of running them
  synchronously inside `fork`.
- Switch runnable user processes from the timer interrupt path.
- Block parents in `wait` when children exist but no child is ready to reap.
- Wake blocked parents from `exit` and reclaim the exited child's user pages.
- Add scheduler-focused user tests: `/bin/multiforktest` and `/bin/schedtest`.

Exit criteria: `make regression` covers `/bin/forktest`,
`/bin/multiforktest`, `/bin/vmtest`, `/bin/schedtest`, shell workflows, and
`ps` process reporting.

Current limitation: M9 remains single-hart and still uses raw linked user
binary images. M10 should replace raw program loading with a real RISC-V32 ELF
loader.

## M10: ELF Loader

- Add a minimal RV32 little-endian ELF parser for executable images and
  loadable program headers.
- Keep raw binary execution active until segment loading is wired into `exec`.

Current status: Step 1 adds parser validation and a boot-time parser self-test.
Step 2 loads ELF `PT_LOAD` segments during `exec` and keeps raw binary fallback
available for compatibility. Step 3 switches the embedded `/bin/*` images and
init shell to ELF blobs.

## M11: VFS

- Add a small VFS syscall layer so user file operations no longer call ramfs
  directly.
- Keep ramfs mounted at `/`.
- Expose `/dev/console` as the first device path. Reads return EOF for now;
  writes go to the serial console.

Current status: Step 1 introduces the VFS wrapper and `/dev/console`. Step 2
moves proc-style files behind dynamic VFS nodes: `/proc/status`,
`/proc/meminfo`, `/proc/uptime`, and `/proc/<pid>`. Step 3 adapts user tools
so `ps` reads `/proc/status`.

## M12: virtio-blk

- Map the QEMU `virt` virtio-mmio transport window.
- Probe virtio-mmio devices and identify virtio block devices when QEMU starts
  with one attached.

Current status: Step 1 detects virtio-mmio transports and reports
`virtio-blk: found` or `virtio-blk: not found`. Step 2 initializes a legacy
virtio-mmio block queue and performs a synchronous sector 0 read when a disk is
attached.
