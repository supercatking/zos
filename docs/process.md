# Process Model

ZOS runs one hardware hart and a fixed-size process table. Each user process has
its own pid, parent pid, trap frame, kernel trap stack, Sv32 page table, user
text pages, user stack page, and process state.

## States

The scheduler uses a small state machine:

- `UNUSED`: free process table slot.
- `RUNNABLE`: ready to run in U-mode.
- `RUNNING`: current user process.
- `BLOCKED`: waiting in a kernel syscall such as `wait`.
- `SLEEPING`: reserved for future blocking sleep work.
- `ZOMBIE`: exited and waiting for the parent to reap it.

## Execution

`exec(path, arg)` loads an embedded RV32 ELF image from `/bin/*`, replaces the
current user image, sets the user entry point and stack, then returns to U-mode.
`fork()` copies the parent's user pages and trap frame into a child process.
Timer interrupts drive round-robin switching between runnable processes.

`wait()` returns a zombie child pid when one exists. If the caller has living
children but none are zombie, the caller is blocked until a child exits. `exit`
marks non-init processes as zombie and wakes a blocked parent.

## Limits

The model has no SMP, priorities, signals, process groups, sessions, job
control, copy-on-write, or resource limits yet.
