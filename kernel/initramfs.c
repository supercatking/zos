#include <zos/console.h>
#include <zos/initramfs.h>

#define INITRAMFS_MAX_OPEN 4

struct initramfs_file {
    const char *path;
    const char *data;
    uintptr_t len;
};

struct open_file {
    const struct initramfs_file *file;
    uintptr_t offset;
};

static const char readme[] =
    "ZOS README\n"
    "A tiny RISC-V32 teaching OS with a user-mode shell.\n";

static const char proc_status[] =
    "pid: 1\n"
    "name: sh\n"
    "state: running\n";

static const struct initramfs_file files[] = {
    {"/README", readme, sizeof(readme) - 1u},
    {"/proc/status", proc_status, sizeof(proc_status) - 1u},
};

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

void initramfs_init(void)
{
    for (int i = 0; i < INITRAMFS_MAX_OPEN; i++) {
        open_files[i].file = 0;
        open_files[i].offset = 0;
    }
    console_puts("initramfs: ready\n");
}

int initramfs_open(const char *path)
{
    const struct initramfs_file *file = 0;

    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        if (streq(path, files[i].path)) {
            file = &files[i];
            break;
        }
    }

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

const char *initramfs_list(void)
{
    return "/README\n/proc/status\n";
}
