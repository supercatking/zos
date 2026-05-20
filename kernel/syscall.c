#include <zos/console.h>
#include <zos/initramfs.h>
#include <zos/pmm.h>
#include <zos/syscall.h>
#include <zos/timer.h>
#include <zos/types.h>

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

    return initramfs_write((int)fd, buf, len);
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

    return initramfs_read((int)fd, buf, len);
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

static uintptr_t write_uint(char *buf, uintptr_t out, uintptr_t len, uintptr_t value)
{
    char tmp[10];
    uintptr_t n = 0;

    if (value == 0) {
        if (out < len) {
            buf[out++] = '0';
        }
        return out;
    }

    while (value != 0 && n < sizeof(tmp)) {
        tmp[n++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (n != 0 && out < len) {
        buf[out++] = tmp[--n];
    }
    return out;
}

static uintptr_t write_str(char *buf, uintptr_t out, uintptr_t len, const char *s)
{
    for (uintptr_t i = 0; s[i] != '\0' && out < len; i++) {
        buf[out++] = s[i];
    }
    return out;
}

static uintptr_t sys_meminfo(char *buf, uintptr_t len)
{
    uintptr_t out = 0;

    out = write_str(buf, out, len, "mem total_pages=");
    out = write_uint(buf, out, len, pmm_total_pages());
    out = write_str(buf, out, len, " free_pages=");
    out = write_uint(buf, out, len, pmm_free_pages());
    if (out < len) {
        buf[out++] = '\n';
    }
    return out;
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
    case SYS_READ:
        tf->a0 = sys_read(tf->a0, (char *)tf->a1, tf->a2);
        break;
    case SYS_OPEN:
        tf->a0 = (uintptr_t)initramfs_open((const char *)tf->a0);
        break;
    case SYS_CLOSE:
        tf->a0 = (uintptr_t)initramfs_close((int)tf->a0);
        break;
    case SYS_SLEEP:
        tf->a0 = sys_sleep(tf->a0);
        break;
    case SYS_KILL:
        tf->a0 = sys_kill(tf->a0);
        break;
    case SYS_CREATE:
        tf->a0 = (uintptr_t)initramfs_create((const char *)tf->a0);
        break;
    case SYS_LIST:
        tf->a0 = initramfs_list((const char *)tf->a2, (char *)tf->a0, tf->a1);
        break;
    case SYS_UNLINK:
        tf->a0 = (uintptr_t)initramfs_unlink((const char *)tf->a0);
        break;
    case SYS_STAT:
        tf->a0 = initramfs_stat((const char *)tf->a0, (char *)tf->a1, tf->a2);
        break;
    case SYS_MKDIR:
        tf->a0 = (uintptr_t)initramfs_mkdir((const char *)tf->a0);
        break;
    case SYS_RENAME:
        tf->a0 = (uintptr_t)initramfs_rename((const char *)tf->a0,
                                             (const char *)tf->a1);
        break;
    case SYS_UPTIME:
        tf->a0 = ((uintptr_t)timer_ticks()) / TIMER_HZ;
        break;
    case SYS_MEMINFO:
        tf->a0 = sys_meminfo((char *)tf->a0, tf->a1);
        break;
    default:
        console_puts("syscall: unknown ");
        console_put_hex(number);
        console_puts("\n");
        tf->a0 = (uintptr_t)-1;
        break;
    }
}
