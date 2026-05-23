#ifndef ZOS_DISKFS_H
#define ZOS_DISKFS_H

#include <zos/block.h>
#include <zos/types.h>

#define DISKFS_MAGIC 0x5a4f5346u
#define DISKFS_VERSION 1u
#define DISKFS_NAME_MAX 28u
#define DISKFS_MAX_INODES 16u
#define DISKFS_DIRECT_BLOCKS 4u
#define DISKFS_ROOT_INO 1u

#define DISKFS_SUPER_SECTOR 1u
#define DISKFS_INODE_SECTOR 2u
#define DISKFS_DIRENT_SECTOR 4u
#define DISKFS_DATA_START_SECTOR 8u

enum diskfs_inode_type {
    DISKFS_INODE_FREE = 0,
    DISKFS_INODE_FILE = 1,
    DISKFS_INODE_DIR = 2,
};

struct diskfs_superblock {
    uint32_t magic;
    uint32_t version;
    uint32_t total_sectors;
    uint32_t inode_count;
    uint32_t data_start_sector;
    uint32_t root_ino;
};

struct diskfs_inode {
    uint32_t type;
    uint32_t size;
    uint32_t blocks[DISKFS_DIRECT_BLOCKS];
};

struct diskfs_dirent {
    uint32_t ino;
    char name[DISKFS_NAME_MAX];
};

void diskfs_mount(void);
int diskfs_mounted(void);
int diskfs_open(const char *path);
int diskfs_create(const char *path);
int diskfs_mkdir(const char *path);
uintptr_t diskfs_read(int fd, char *buf, uintptr_t len);
uintptr_t diskfs_write(int fd, const char *buf, uintptr_t len);
int diskfs_close(int fd);
uintptr_t diskfs_list(const char *path, char *buf, uintptr_t len);
int diskfs_unlink(const char *path);
int diskfs_rename(const char *old_path, const char *new_path);
uintptr_t diskfs_stat(const char *path, char *buf, uintptr_t len);

#endif
