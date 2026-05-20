#include <zos/console.h>
#include <zos/initramfs.h>

#define INITRAMFS_MAX_OPEN 4
#define RAMFS_MAX_FILES 8
#define RAMFS_NAME_MAX 32
#define RAMFS_DATA_MAX 256

struct initramfs_file {
    char path[RAMFS_NAME_MAX];
    char data[RAMFS_DATA_MAX];
    uintptr_t len;
    int used;
    int writable;
};

struct open_file {
    struct initramfs_file *file;
    uintptr_t offset;
};

static const char readme[] =
    "ZOS README\n"
    "A tiny RISC-V32 teaching OS with a user-mode shell.\n";

static const char proc_status[] =
    "pid: 1\n"
    "name: sh\n"
    "state: running\n";

static struct initramfs_file files[RAMFS_MAX_FILES];
static struct open_file open_files[INITRAMFS_MAX_OPEN];

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

static void copy_string(char *dst, const char *src, uintptr_t max)
{
    uintptr_t i = 0;

    if (max == 0) {
        return;
    }

    while (i + 1u < max && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static void copy_data(char *dst, const char *src, uintptr_t len)
{
    for (uintptr_t i = 0; i < len; i++) {
        dst[i] = src[i];
    }
}

static struct initramfs_file *find_file(const char *path)
{
    for (size_t i = 0; i < RAMFS_MAX_FILES; i++) {
        if (files[i].used && streq(path, files[i].path)) {
            return &files[i];
        }
    }
    return 0;
}

static void add_static_file(const char *path, const char *data, uintptr_t len)
{
    for (size_t i = 0; i < RAMFS_MAX_FILES; i++) {
        if (!files[i].used) {
            files[i].used = 1;
            files[i].writable = 0;
            files[i].len = len;
            copy_string(files[i].path, path, sizeof(files[i].path));
            copy_data(files[i].data, data, len);
            return;
        }
    }
}

void initramfs_init(void)
{
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        files[i].used = 0;
        files[i].len = 0;
        files[i].writable = 0;
    }

    for (int i = 0; i < INITRAMFS_MAX_OPEN; i++) {
        open_files[i].file = 0;
        open_files[i].offset = 0;
    }

    add_static_file("/README", readme, sizeof(readme) - 1u);
    add_static_file("/proc/status", proc_status, sizeof(proc_status) - 1u);
    console_puts("initramfs: ready\n");
}

int initramfs_open(const char *path)
{
    struct initramfs_file *file = find_file(path);

    if (file == 0) {
        return -1;
    }

    for (int i = 0; i < INITRAMFS_MAX_OPEN; i++) {
        if (open_files[i].file == 0) {
            open_files[i].file = file;
            open_files[i].offset = 0;
            return i + 3;
        }
    }

    return -1;
}

int initramfs_create(const char *path)
{
    struct initramfs_file *file = find_file(path);

    if (file != 0) {
        file->len = 0;
        return 0;
    }

    for (size_t i = 0; i < RAMFS_MAX_FILES; i++) {
        if (!files[i].used) {
            files[i].used = 1;
            files[i].writable = 1;
            files[i].len = 0;
            copy_string(files[i].path, path, sizeof(files[i].path));
            return 0;
        }
    }

    return -1;
}

uintptr_t initramfs_read(int fd, char *buf, uintptr_t len)
{
    int slot = fd - 3;
    uintptr_t count = 0;

    if (slot < 0 || slot >= INITRAMFS_MAX_OPEN || open_files[slot].file == 0) {
        return (uintptr_t)-1;
    }

    while (count < len && open_files[slot].offset < open_files[slot].file->len) {
        buf[count++] = open_files[slot].file->data[open_files[slot].offset++];
    }

    return count;
}

uintptr_t initramfs_write(int fd, const char *buf, uintptr_t len)
{
    int slot = fd - 3;
    uintptr_t count = 0;

    if (slot < 0 || slot >= INITRAMFS_MAX_OPEN || open_files[slot].file == 0) {
        return (uintptr_t)-1;
    }

    if (!open_files[slot].file->writable) {
        return (uintptr_t)-1;
    }

    while (count < len && open_files[slot].offset < RAMFS_DATA_MAX) {
        open_files[slot].file->data[open_files[slot].offset++] = buf[count++];
    }
    if (open_files[slot].offset > open_files[slot].file->len) {
        open_files[slot].file->len = open_files[slot].offset;
    }
    return count;
}

int initramfs_close(int fd)
{
    int slot = fd - 3;

    if (slot < 0 || slot >= INITRAMFS_MAX_OPEN || open_files[slot].file == 0) {
        return -1;
    }

    open_files[slot].file = 0;
    open_files[slot].offset = 0;
    return 0;
}

uintptr_t initramfs_list(char *buf, uintptr_t len)
{
    uintptr_t out = 0;

    for (size_t i = 0; i < RAMFS_MAX_FILES; i++) {
        if (!files[i].used) {
            continue;
        }
        for (uintptr_t j = 0; files[i].path[j] != '\0' && out < len; j++) {
            buf[out++] = files[i].path[j];
        }
        if (out < len) {
            buf[out++] = '\n';
        }
    }

    return out;
}
