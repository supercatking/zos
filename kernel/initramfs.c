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
    int is_dir;
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
        tmp[n++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (n != 0) {
        out = append_char(buf, out, len, tmp[--n]);
    }
    return out;
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
            files[i].is_dir = 0;
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
        files[i].is_dir = 0;
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

    if (file == 0 || file->is_dir) {
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
        if (file->is_dir) {
            return -1;
        }
        file->len = 0;
        file->writable = 1;
        return 0;
    }

    for (size_t i = 0; i < RAMFS_MAX_FILES; i++) {
        if (!files[i].used) {
            files[i].used = 1;
            files[i].writable = 1;
            files[i].is_dir = 0;
            files[i].len = 0;
            copy_string(files[i].path, path, sizeof(files[i].path));
            return 0;
        }
    }

    return -1;
}

int initramfs_mkdir(const char *path)
{
    if (find_file(path) != 0) {
        return -1;
    }

    for (size_t i = 0; i < RAMFS_MAX_FILES; i++) {
        if (!files[i].used) {
            files[i].used = 1;
            files[i].writable = 0;
            files[i].is_dir = 1;
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

    if (slot < 0 || slot >= INITRAMFS_MAX_OPEN || open_files[slot].file == 0 ||
        open_files[slot].file->is_dir) {
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

    if (!open_files[slot].file->writable || open_files[slot].file->is_dir) {
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

int initramfs_unlink(const char *path)
{
    struct initramfs_file *file = find_file(path);

    if (file == 0 || !file->writable) {
        return -1;
    }

    if (file->is_dir) {
        for (size_t i = 0; i < RAMFS_MAX_FILES; i++) {
            if (files[i].used && &files[i] != file &&
                starts_with(files[i].path, file->path)) {
                const char *tail = files[i].path;
                const char *dir = file->path;
                while (*dir != '\0') {
                    tail++;
                    dir++;
                }
                if (*tail == '/') {
                    return -1;
                }
            }
        }
    }

    file->used = 0;
    file->len = 0;
    file->path[0] = '\0';
    return 0;
}

int initramfs_rename(const char *old_path, const char *new_path)
{
    struct initramfs_file *old_file = find_file(old_path);

    if (old_file == 0 || find_file(new_path) != 0 || !old_file->writable) {
        return -1;
    }

    copy_string(old_file->path, new_path, sizeof(old_file->path));
    return 0;
}

uintptr_t initramfs_stat(const char *path, char *buf, uintptr_t len)
{
    struct initramfs_file *file = find_file(path);
    uintptr_t out = 0;

    if (file == 0) {
        return (uintptr_t)-1;
    }

    out = append_str(buf, out, len, file->is_dir ? "dir " : "file ");
    out = append_str(buf, out, len, file->path);
    out = append_str(buf, out, len, " size=");
    out = append_uint(buf, out, len, file->len);
    out = append_char(buf, out, len, '\n');
    return out;
}
