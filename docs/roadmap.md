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
