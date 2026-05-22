#include <zos/console.h>
#include <zos/syscall.h>
#include <zos/timer.h>
#include <zos/types.h>
#include <zos/user.h>
#include <zos/vfs.h>

#define TIMEBASE_HZ 10000000ull
#define SLEEP_TICK_HZ 100ull

static uint64_t read_time(void)
{
    uint32_t hi;
    uint32_t lo;
    uint32_t hi2;

    do {
        __asm__ volatile("rdtimeh %0" : "=r"(hi));
        __asm__ volatile("rdtime %0" : "=r"(lo));
        __asm__ volatile("rdtimeh %0" : "=r"(hi2));
    } while (hi != hi2);

    return ((uint64_t)hi << 32) | lo;
}

static uintptr_t sys_write(uintptr_t fd, const char *buf, uintptr_t len)
{
    if (fd == 1 || fd == 2) {
        for (uintptr_t i = 0; i < len; i++) {
            console_putchar(buf[i]);
        }
        return len;
    }

    return vfs_write((int)fd, buf, len);
}

static uintptr_t sys_read(uintptr_t fd, char *buf, uintptr_t len)
{
    uintptr_t count = 0;

    if (fd == 0) {
        while (count < len) {
            char ch = console_getchar();
            if (ch == '\r') {
                ch = '\n';
            }
            console_putchar(ch);
            buf[count++] = ch;
            if (ch == '\n') {
                break;
            }
        }
        return count;
    }

    return vfs_read((int)fd, buf, len);
}

static void sys_exit(uintptr_t status, struct trap_frame *tf)
{
    int exit_result = user_exit_process(status, tf);

    if (exit_result > 0) {
        return;
    }

    if (!user_current_is_shell()) {
        if (user_exec("/bin/sh", "", tf) != 0) {
            console_puts("user: restart shell failed\n");
            for (;;) {
                __asm__ volatile("wfi");
            }
        }
        return;
    }

    console_puts("user: exit status ");
    console_put_hex(status);
    console_puts("\n");
    console_puts("user: halted cleanly\n");

    for (;;) {
        __asm__ volatile("wfi");
    }
}

static uintptr_t sys_sleep(uintptr_t ticks)
{
    uint64_t interval = (TIMEBASE_HZ / SLEEP_TICK_HZ) * (uint64_t)ticks;
    uint64_t until = read_time() + interval;

    while (read_time() < until) {
        __asm__ volatile("nop");
    }

    return 0;
}

static uintptr_t sys_kill(uintptr_t pid)
{
    if (pid == 1) {
        return 0;
    }
    return (uintptr_t)-1;
}

static uintptr_t sys_meminfo(char *buf, uintptr_t len)
{
    int fd = vfs_open("/proc/meminfo");
    uintptr_t n;

    if (fd < 0) {
        return (uintptr_t)-1;
    }
    n = vfs_read(fd, buf, len);
    (void)vfs_close(fd);
    return n;
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
        sys_exit(tf->a0, tf);
        break;
    case SYS_READ:
        tf->a0 = sys_read(tf->a0, (char *)tf->a1, tf->a2);
        break;
    case SYS_OPEN:
        tf->a0 = (uintptr_t)vfs_open((const char *)tf->a0);
        break;
    case SYS_CLOSE:
        tf->a0 = (uintptr_t)vfs_close((int)tf->a0);
        break;
    case SYS_SLEEP:
        tf->a0 = sys_sleep(tf->a0);
        break;
    case SYS_KILL:
        tf->a0 = sys_kill(tf->a0);
        break;
    case SYS_CREATE:
        tf->a0 = (uintptr_t)vfs_create((const char *)tf->a0);
        break;
    case SYS_LIST:
        tf->a0 = vfs_list((const char *)tf->a2, (char *)tf->a0, tf->a1);
        break;
    case SYS_UNLINK:
        tf->a0 = (uintptr_t)vfs_unlink((const char *)tf->a0);
        break;
    case SYS_STAT:
        tf->a0 = vfs_stat((const char *)tf->a0, (char *)tf->a1, tf->a2);
        break;
    case SYS_MKDIR:
        tf->a0 = (uintptr_t)vfs_mkdir((const char *)tf->a0);
        break;
    case SYS_RENAME:
        tf->a0 = (uintptr_t)vfs_rename((const char *)tf->a0,
                                       (const char *)tf->a1);
        break;
    case SYS_UPTIME:
        tf->a0 = ((uintptr_t)timer_ticks()) / TIMER_HZ;
        break;
    case SYS_MEMINFO:
        tf->a0 = sys_meminfo((char *)tf->a0, tf->a1);
        break;
    case SYS_EXEC:
        if (user_exec((const char *)tf->a0, (const char *)tf->a1, tf) != 0) {
            tf->a0 = (uintptr_t)-1;
        }
        break;
    case SYS_FORK:
        tf->a0 = (uintptr_t)user_fork(tf);
        break;
    case SYS_WAIT:
    {
        int wait_result = user_wait(tf);
        if (wait_result != -2) {
            tf->a0 = (uintptr_t)wait_result;
        }
        break;
    }
    case SYS_GETPID:
        tf->a0 = (uintptr_t)user_getpid();
        break;
    case SYS_PROCINFO:
        tf->a0 = user_procinfo((char *)tf->a0, tf->a1);
        break;
    default:
        console_puts("syscall: unknown ");
        console_put_hex(number);
        console_puts("\n");
        tf->a0 = (uintptr_t)-1;
        break;
    }
}
