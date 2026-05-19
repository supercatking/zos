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
- Add simple assertions or smoke tests for trap handling.

Exit criteria: illegal instruction, breakpoint, and timer paths are observable
and do not corrupt kernel state.

## M3: Memory Management

- Define physical memory ranges for the QEMU `virt` machine.
- Add a page allocator for fixed-size pages.
- Introduce kernel virtual memory if the teaching path uses paging at this
  stage.
- Document memory layout and ownership rules.

Exit criteria: kernel allocations are repeatable, bounded, and testable.

## M4: User Mode and System Calls

- Add user address space setup and transition from kernel to user mode.
- Load simple user programs from linked-in images or an initramfs.
- Implement a small syscall ABI for `write`, `read`, `exit`, `wait`, `fork`,
  `exec`, `open`, and `close`.
- Validate user pointers before the kernel touches user memory.

Exit criteria: a user program can print through a syscall and exit cleanly.

## M5: Initramfs and User Shell

- Add a tiny read-only initramfs image.
- Provide file-descriptor backed `open`, `read`, and `close`.
- Build a user-mode shell using the serial console as its terminal.
- Add first commands: `help`, `echo`, `cat`, `ls`, `ps`, and `reboot`.

Exit criteria: QEMU boots into a user-facing shell that can run simple commands.

## M6: Linux-like Extensions

- Extend process control with `sleep`, `kill`, and richer exit status.
- Add a VFS layer that can expose initramfs, console, and proc-like nodes.
- Prepare virtio-blk as the next storage backend without blocking the shell.
- Add regression tests for multi-program shell workflows.

Exit criteria: QEMU can run several small user programs and report process
state from the shell.
