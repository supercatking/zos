#include <zos/block.h>
#include <zos/console.h>
#include <zos/diskfs.h>
#include <zos/types.h>

#define DISKFS_FD_BASE 130
#define DISKFS_FD_LIMIT 134
#define DISKFS_OPEN_MAX 4

struct disk_open {
    int used;
    uint32_t ino;
    uintptr_t offset;
};

static int mounted;
static struct diskfs_superblock super;
static struct diskfs_inode inodes[DISKFS_MAX_INODES];
static struct diskfs_dirent dirents[DISKFS_MAX_INODES];
static struct disk_open open_files[DISKFS_OPEN_MAX];

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

static void copy_bytes(void *dst, const void *src, uintptr_t len)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;

    for (uintptr_t i = 0; i < len; i++) {
        d[i] = s[i];
    }
}

static void zero_bytes(void *ptr, uintptr_t len)
{
    uint8_t *p = (uint8_t *)ptr;

    for (uintptr_t i = 0; i < len; i++) {
        p[i] = 0;
    }
}

static void copy_name(char *dst, const char *src)
{
    uintptr_t i = 0;

    while (i + 1u < DISKFS_NAME_MAX && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static int disk_path_name(const char *path, const char **name)
{
    const char *prefix = "/disk/";

    if (path == 0 || name == 0) {
        return -1;
    }
    for (uintptr_t i = 0; prefix[i] != '\0'; i++) {
        if (path[i] != prefix[i]) {
            return -1;
        }
    }
    *name = path + 6;
    return (*name)[0] == '\0' ? -1 : 0;
}

static int find_dirent(const char *name)
{
    for (int i = 0; i < (int)DISKFS_MAX_INODES; i++) {
        if (dirents[i].ino != 0 && streq(dirents[i].name, name)) {
            return i;
        }
    }
    return -1;
}

static int fd_slot(int fd)
{
    int slot = fd - DISKFS_FD_BASE;

    if (slot < 0 || slot >= DISKFS_OPEN_MAX || !open_files[slot].used) {
        return -1;
    }
    return slot;
}

static int sync_metadata(void)
{
    uint8_t sector[BLOCK_SECTOR_SIZE];

    zero_bytes(sector, sizeof(sector));
    copy_bytes(sector, inodes, sizeof(inodes));
    if (block_write_sector(DISKFS_INODE_SECTOR, sector) != 0) {
        return -1;
    }

    zero_bytes(sector, sizeof(sector));
    copy_bytes(sector, dirents, sizeof(dirents));
    return block_write_sector(DISKFS_DIRENT_SECTOR, sector);
}

void diskfs_mount(void)
{
    uint8_t sector[BLOCK_SECTOR_SIZE];

    mounted = 0;
    if (block_read_sector(DISKFS_SUPER_SECTOR, sector) != 0) {
        console_puts("diskfs: no disk\n");
        return;
    }

    copy_bytes(&super, sector, sizeof(super));
    if (super.magic != DISKFS_MAGIC ||
        super.version != DISKFS_VERSION ||
        super.inode_count > DISKFS_MAX_INODES ||
        super.root_ino != DISKFS_ROOT_INO) {
        console_puts("diskfs: not mounted\n");
        return;
    }

    if (block_read_sector(DISKFS_INODE_SECTOR, sector) != 0) {
        console_puts("diskfs: inode read failed\n");
        return;
    }
    copy_bytes(inodes, sector, sizeof(inodes));

    if (block_read_sector(DISKFS_DIRENT_SECTOR, sector) != 0) {
        console_puts("diskfs: dir read failed\n");
        return;
    }
    copy_bytes(dirents, sector, sizeof(dirents));

    for (int i = 0; i < DISKFS_OPEN_MAX; i++) {
        open_files[i].used = 0;
    }
    mounted = 1;
    console_puts("diskfs: mounted at /disk\n");
}

int diskfs_mounted(void)
{
    return mounted;
}

int diskfs_open(const char *path)
{
    const char *name;
    int dirent_index;
    uint32_t ino;

    if (!mounted || disk_path_name(path, &name) != 0) {
        return -1;
    }

    dirent_index = find_dirent(name);
    if (dirent_index < 0) {
        return -1;
    }

    ino = dirents[dirent_index].ino;
    if (ino == 0 || ino > DISKFS_MAX_INODES ||
        inodes[ino - 1u].type != DISKFS_INODE_FILE) {
        return -1;
    }

    for (int i = 0; i < DISKFS_OPEN_MAX; i++) {
        if (!open_files[i].used) {
            open_files[i].used = 1;
            open_files[i].ino = ino;
            open_files[i].offset = 0;
            return DISKFS_FD_BASE + i;
        }
    }
    return -1;
}

int diskfs_create(const char *path)
{
    const char *name;
    int dirent_index;
    int inode_index = -1;

    if (!mounted || disk_path_name(path, &name) != 0) {
        return -1;
    }

    dirent_index = find_dirent(name);
    if (dirent_index >= 0) {
        uint32_t ino = dirents[dirent_index].ino;
        inodes[ino - 1u].size = 0;
        return sync_metadata();
    }

    for (int i = 1; i < (int)DISKFS_MAX_INODES; i++) {
        if (inodes[i].type == DISKFS_INODE_FREE) {
            inode_index = i;
            break;
        }
    }
    if (inode_index < 0) {
        return -1;
    }

    for (int i = 0; i < (int)DISKFS_MAX_INODES; i++) {
        if (dirents[i].ino == 0) {
            dirent_index = i;
            break;
        }
    }
    if (dirent_index < 0) {
        return -1;
    }

    inodes[inode_index].type = DISKFS_INODE_FILE;
    inodes[inode_index].size = 0;
    inodes[inode_index].blocks[0] = DISKFS_DATA_START_SECTOR + (uint32_t)inode_index;
    dirents[dirent_index].ino = (uint32_t)inode_index + 1u;
    copy_name(dirents[dirent_index].name, name);
    return sync_metadata();
}

uintptr_t diskfs_read(int fd, char *buf, uintptr_t len)
{
    int slot = fd_slot(fd);
    uint8_t sector[BLOCK_SECTOR_SIZE];
    struct diskfs_inode *inode;
    uintptr_t count = 0;

    if (slot < 0) {
        return (uintptr_t)-1;
    }

    inode = &inodes[open_files[slot].ino - 1u];
    if (block_read_sector(inode->blocks[0], sector) != 0) {
        return (uintptr_t)-1;
    }

    while (count < len && open_files[slot].offset < inode->size) {
        buf[count++] = sector[open_files[slot].offset++];
    }
    return count;
}

uintptr_t diskfs_write(int fd, const char *buf, uintptr_t len)
{
    int slot = fd_slot(fd);
    uint8_t sector[BLOCK_SECTOR_SIZE];
    struct diskfs_inode *inode;
    uintptr_t count = 0;

    if (slot < 0) {
        return (uintptr_t)-1;
    }

    inode = &inodes[open_files[slot].ino - 1u];
    zero_bytes(sector, sizeof(sector));
    (void)block_read_sector(inode->blocks[0], sector);
    while (count < len && open_files[slot].offset < BLOCK_SECTOR_SIZE) {
        sector[open_files[slot].offset++] = buf[count++];
    }
    if (open_files[slot].offset > inode->size) {
        inode->size = open_files[slot].offset;
    }
    if (block_write_sector(inode->blocks[0], sector) != 0 || sync_metadata() != 0) {
        return (uintptr_t)-1;
    }
    return count;
}

int diskfs_close(int fd)
{
    int slot = fd_slot(fd);

    if (slot < 0) {
        return -1;
    }
    open_files[slot].used = 0;
    return 0;
}

uintptr_t diskfs_list(char *buf, uintptr_t len)
{
    uintptr_t out = 0;

    if (!mounted) {
        return (uintptr_t)-1;
    }

    for (int i = 0; i < (int)DISKFS_MAX_INODES; i++) {
        if (dirents[i].ino != 0) {
            out = append_str(buf, out, len, dirents[i].name);
            out = append_char(buf, out, len, '\n');
        }
    }
    return out;
}

uintptr_t diskfs_stat(const char *path, char *buf, uintptr_t len)
{
    const char *name;
    uintptr_t out = 0;
    int dirent_index;
    uint32_t ino;

    if (!mounted || disk_path_name(path, &name) != 0) {
        return (uintptr_t)-1;
    }

    dirent_index = find_dirent(name);
    if (dirent_index < 0) {
        return (uintptr_t)-1;
    }

    ino = dirents[dirent_index].ino;
    out = append_str(buf, out, len, "disk file ");
    out = append_str(buf, out, len, path);
    out = append_str(buf, out, len, " size=");
    out = append_uint(buf, out, len, inodes[ino - 1u].size);
    out = append_char(buf, out, len, '\n');
    return out;
}
