#include <zos/console.h>
#include <zos/panic.h>
#include <zos/riscv.h>
#include <zos/syscall.h>
#include <zos/timer.h>
#include <zos/trap.h>

#define TRAP_KERNEL_STACK_SIZE 4096u

static uint8_t trap_kernel_stack[TRAP_KERNEL_STACK_SIZE]
    __attribute__((aligned(16)));

void timer_interrupt(struct trap_frame *tf) __attribute__((weak));

uintptr_t trap_kernel_stack_top(void)
{
    return (uintptr_t)&trap_kernel_stack[TRAP_KERNEL_STACK_SIZE];
}

void trap_init(void)
{
    w_stvec((uintptr_t)trap_entry | STVEC_MODE_DIRECT);
    w_sscratch(0);
}

void trap_handler(struct trap_frame *tf)
{
    uintptr_t scause = tf->scause;
    uintptr_t code = scause & SCAUSE_CODE_MASK;

    if ((scause & SCAUSE_INTERRUPT) != 0 &&
        code == SCAUSE_SUPERVISOR_TIMER_INTERRUPT) {
        if (timer_interrupt != 0) {
            timer_interrupt(tf);
        } else {
            timer_handle_interrupt();
        }
        return;
    }

    if ((scause & SCAUSE_INTERRUPT) == 0 && code == SCAUSE_USER_ECALL) {
        syscall_handle(tf);
        return;
    }

    console_puts("trap: scause=");
    console_put_hex(scause);
    console_puts(" stval=");
    console_put_hex(tf->stval);
    console_puts(" sepc=");
    console_put_hex(tf->sepc);
    console_puts("\n");

    PANIC("unhandled supervisor trap");
}
