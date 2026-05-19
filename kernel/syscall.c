#include <zos/console.h>
#include <zos/syscall.h>
#include <zos/types.h>

static uintptr_t sys_write(uintptr_t fd, const char *buf, uintptr_t len)
{
    if (fd != 1 && fd != 2) {
        return (uintptr_t)-1;
    }

    for (uintptr_t i = 0; i < len; i++) {
        console_putchar(buf[i]);
    }

    return len;
}

static void sys_exit(uintptr_t status)
{
    console_puts("user: exit status ");
    console_put_hex(status);
    console_puts("\n");
    console_puts("user: halted cleanly\n");

    for (;;) {
        __asm__ volatile("wfi");
    }
}

void syscall_handle(struct trap_frame *tf)
{
    uintptr_t number = tf->a7;

    tf->sepc += 4;

    switch (number) {
    case SYS_WRITE:
        tf->a0 = sys_write(tf->a0, (const char *)tf->a1, tf->a2);
        break;
    case SYS_EXIT:
        sys_exit(tf->a0);
        break;
    default:
        console_puts("syscall: unknown ");
        console_put_hex(number);
        console_puts("\n");
        tf->a0 = (uintptr_t)-1;
        break;
    }
}
