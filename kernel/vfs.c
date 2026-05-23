#include <zos/console.h>
#include <zos/diskfs.h>
#include <zos/initramfs.h>
#include <zos/pmm.h>
#include <zos/timer.h>
#include <zos/types.h>
#include <zos/user.h>
#include <zos/vfs.h>

#define VFS_CONSOLE_FD 100
#define VFS_PROC_FD_BASE 110
#define VFS_PROC_OPEN 4
#define VFS_PROC_DATA_MAX 192

struct proc_file {
    int used;
    uintptr_t offset;
    uintptr_t len;
    char data[VFS_PROC_DATA_MAX];
};

static struct proc_file proc_files[VFS_PROC_OPEN];

static int streq(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static uintptr_t append_char(char *buf, uintptr_t out, uintptr_t len, char ch)
{
    if (out < len) {
        buf[out++] = ch;
    }
    return out;
}

static uintptr_t append_str(char *buf, uintptr_t out, uintptr_t len, const char *s)
{
    for (uintptr_t i = 0; s[i] != '\0'; i++) {
        out = append_char(buf, out, len, s[i]);
    }
    return out;
}

static uintptr_t append_uint(char *buf, uintptr_t out, uintptr_t len, uintptr_t value)
{
    char tmp[10];
    uintptr_t n = 0;

    if (value == 0) {
        return append_char(buf, out, len, '0');
    }

    while (value != 0 && n < sizeof(tmp)) {
        tmp[n++] = (char)('0' + value % 10u);
        value /= 10u;
    }
    while (n != 0) {
        out = append_char(buf, out, len, tmp[--n]);
    }
    return out;
}

static int starts_with(const char *s, const char *prefix)
{
    while (*prefix != '\0') {
        if (*s != *prefix) {
            return 0;
        }
        s++;
        prefix++;
    }
    return 1;
}

static int all_digits(const char *s)
{
    if (*s == '\0') {
        return 0;
    }

    while (*s != '\0') {
        if (*s < '0' || *s > '9') {
            return 0;
        }
        s++;
    }
    return 1;
}

static uintptr_t proc_meminfo(char *buf, uintptr_t len)
{
    uintptr_t out = 0;

    out = append_str(buf, out, len, "mem total_pages=");
    out = append_uint(buf, out, len, pmm_total_pages());
    out = append_str(buf, out, len, " free_pages=");
    out = append_uint(buf, out, len, pmm_free_pages());
    out = append_char(buf, out, len, '\n');
    return out;
}

static uintptr_t proc_uptime(char *buf, uintptr_t len)
{
    uintptr_t out = 0;

    out = append_str(buf, out, len, "uptime ");
    out = append_uint(buf, out, len, ((uintptr_t)timer_ticks()) / TIMER_HZ);
    out = append_str(buf, out, len, "s\n");
    return out;
}

static uintptr_t proc_pid(const char *path, char *buf, uintptr_t len)
{
    uintptr_t out = 0;

    out = append_str(buf, out, len, "pid: ");
    out = append_str(buf, out, len, path + 6);
    out = append_char(buf, out, len, '\n');
    return out;
}

static int proc_fill(const char *path, char *buf, uintptr_t len)
{
    if (streq(path, "/proc/status")) {
        return (int)user_procinfo(buf, len);
    }
    if (streq(path, "/proc/meminfo")) {
        return (int)proc_meminfo(buf, len);
    }
    if (streq(path, "/proc/uptime")) {
        return (int)proc_uptime(buf, len);
    }
    if (starts_with(path, "/proc/") && all_digits(path + 6)) {
        return (int)proc_pid(path, buf, len);
    }
    return -1;
}

static int proc_open(const char *path)
{
    for (int i = 0; i < VFS_PROC_OPEN; i++) {
        if (!proc_files[i].used) {
            int n = proc_fill(path, proc_files[i].data, sizeof(proc_files[i].data));
            if (n < 0) {
                return -1;
            }
            proc_files[i].used = 1;
            proc_files[i].offset = 0;
            proc_files[i].len = (uintptr_t)n;
            return VFS_PROC_FD_BASE + i;
        }
    }
    return -1;
}

static int proc_fd_slot(int fd)
{
    int slot = fd - VFS_PROC_FD_BASE;

    if (slot < 0 || slot >= VFS_PROC_OPEN || !proc_files[slot].used) {
        return -1;
    }
    return slot;
}

void vfs_init(void)
{
    for (int i = 0; i < VFS_PROC_OPEN; i++) {
        proc_files[i].used = 0;
    }

    console_puts("vfs: ramfs mounted at /\n");
    console_puts("vfs: console mounted at /dev/console\n");
    console_puts("vfs: procfs mounted at /proc\n");
}

int vfs_open(const char *path)
{
    if (path != 0 && streq(path, "/dev/console")) {
        return VFS_CONSOLE_FD;
    }
    if (path != 0 && starts_with(path, "/proc/")) {
        return proc_open(path);
    }
    if (path != 0 && starts_with(path, "/disk/")) {
        return diskfs_open(path);
    }

    return initramfs_open(path);
}

int vfs_create(const char *path)
{
    if (path != 0 && streq(path, "/dev/console")) {
        return -1;
    }
    if (path != 0 && starts_with(path, "/disk/")) {
        return diskfs_create(path);
    }

    return initramfs_create(path);
}

int vfs_mkdir(const char *path)
{
    if (path != 0 && starts_with(path, "/disk/")) {
        return diskfs_mkdir(path);
    }

    return initramfs_mkdir(path);
}

uintptr_t vfs_read(int fd, char *buf, uintptr_t len)
{
    (void)buf;
    (void)len;

    if (fd == VFS_CONSOLE_FD) {
        return 0;
    }
    if (fd >= VFS_PROC_FD_BASE && fd < VFS_PROC_FD_BASE + VFS_PROC_OPEN) {
        int slot = proc_fd_slot(fd);
        uintptr_t count = 0;

        if (slot < 0) {
            return (uintptr_t)-1;
        }

        while (count < len && proc_files[slot].offset < proc_files[slot].len) {
            buf[count++] = proc_files[slot].data[proc_files[slot].offset++];
        }
        return count;
    }
    if (fd >= 130 && fd < 134) {
        return diskfs_read(fd, buf, len);
    }

    return initramfs_read(fd, buf, len);
}

uintptr_t vfs_write(int fd, const char *buf, uintptr_t len)
{
    if (fd == VFS_CONSOLE_FD) {
        for (uintptr_t i = 0; i < len; i++) {
            console_putchar(buf[i]);
        }
        return len;
    }
    if (fd >= 130 && fd < 134) {
        return diskfs_write(fd, buf, len);
    }

    return initramfs_write(fd, buf, len);
}

int vfs_close(int fd)
{
    if (fd == VFS_CONSOLE_FD) {
        return 0;
    }
    if (fd >= VFS_PROC_FD_BASE && fd < VFS_PROC_FD_BASE + VFS_PROC_OPEN) {
        int slot = proc_fd_slot(fd);

        if (slot < 0) {
            return -1;
        }
        proc_files[slot].used = 0;
        proc_files[slot].offset = 0;
        proc_files[slot].len = 0;
        return 0;
    }
    if (fd >= 130 && fd < 134) {
        return diskfs_close(fd);
    }

    return initramfs_close(fd);
}

uintptr_t vfs_list(const char *path, char *buf, uintptr_t len)
{
    if (path != 0 && streq(path, "/dev")) {
        uintptr_t out = 0;
        out = append_str(buf, out, len, "console\n");
        return out;
    }
    if (path != 0 && streq(path, "/proc")) {
        uintptr_t out = 0;
        out = append_str(buf, out, len, "status\n");
        out = append_str(buf, out, len, "meminfo\n");
        out = append_str(buf, out, len, "uptime\n");
        out = append_str(buf, out, len, "1\n");
        return out;
    }
    if (path != 0 && streq(path, "/disk")) {
        return diskfs_list(path, buf, len);
    }
    if (path != 0 && starts_with(path, "/disk/")) {
        return diskfs_list(path, buf, len);
    }

    return initramfs_list(path, buf, len);
}

int vfs_unlink(const char *path)
{
    if (path != 0 && streq(path, "/dev/console")) {
        return -1;
    }
    if (path != 0 && starts_with(path, "/disk/")) {
        return diskfs_unlink(path);
    }

    return initramfs_unlink(path);
}

int vfs_rename(const char *old_path, const char *new_path)
{
    if ((old_path != 0 && streq(old_path, "/dev/console")) ||
        (new_path != 0 && streq(new_path, "/dev/console"))) {
        return -1;
    }
    if (old_path != 0 && new_path != 0 &&
        starts_with(old_path, "/disk/") && starts_with(new_path, "/disk/")) {
        return diskfs_rename(old_path, new_path);
    }

    return initramfs_rename(old_path, new_path);
}

uintptr_t vfs_stat(const char *path, char *buf, uintptr_t len)
{
    uintptr_t out = 0;

    if (path != 0 && streq(path, "/dev/console")) {
        out = append_str(buf, out, len, "char /dev/console size=0\n");
        return out;
    }
    if (path != 0 && starts_with(path, "/proc/") && proc_fill(path, buf, len) >= 0) {
        out = append_str(buf, out, len, "proc ");
        out = append_str(buf, out, len, path);
        out = append_str(buf, out, len, " size=dynamic\n");
        return out;
    }
    if (path != 0 && starts_with(path, "/disk/")) {
        return diskfs_stat(path, buf, len);
    }

    return initramfs_stat(path, buf, len);
}
