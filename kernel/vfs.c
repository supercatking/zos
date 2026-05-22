#include <zos/console.h>
#include <zos/initramfs.h>
#include <zos/types.h>
#include <zos/vfs.h>

#define VFS_CONSOLE_FD 100

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

void vfs_init(void)
{
    console_puts("vfs: ramfs mounted at /\n");
    console_puts("vfs: console mounted at /dev/console\n");
}

int vfs_open(const char *path)
{
    if (path != 0 && streq(path, "/dev/console")) {
        return VFS_CONSOLE_FD;
    }

    return initramfs_open(path);
}

int vfs_create(const char *path)
{
    if (path != 0 && streq(path, "/dev/console")) {
        return -1;
    }

    return initramfs_create(path);
}

int vfs_mkdir(const char *path)
{
    return initramfs_mkdir(path);
}

uintptr_t vfs_read(int fd, char *buf, uintptr_t len)
{
    (void)buf;
    (void)len;

    if (fd == VFS_CONSOLE_FD) {
        return 0;
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

    return initramfs_write(fd, buf, len);
}

int vfs_close(int fd)
{
    if (fd == VFS_CONSOLE_FD) {
        return 0;
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

    return initramfs_list(path, buf, len);
}

int vfs_unlink(const char *path)
{
    if (path != 0 && streq(path, "/dev/console")) {
        return -1;
    }

    return initramfs_unlink(path);
}

int vfs_rename(const char *old_path, const char *new_path)
{
    if ((old_path != 0 && streq(old_path, "/dev/console")) ||
        (new_path != 0 && streq(new_path, "/dev/console"))) {
        return -1;
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

    return initramfs_stat(path, buf, len);
}
