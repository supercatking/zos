#ifndef ZOS_VFS_H
#define ZOS_VFS_H

#include <zos/types.h>

void vfs_init(void);
int vfs_open(const char *path);
int vfs_create(const char *path);
int vfs_mkdir(const char *path);
uintptr_t vfs_read(int fd, char *buf, uintptr_t len);
uintptr_t vfs_write(int fd, const char *buf, uintptr_t len);
int vfs_close(int fd);
uintptr_t vfs_list(const char *path, char *buf, uintptr_t len);
int vfs_unlink(const char *path);
int vfs_rename(const char *old_path, const char *new_path);
uintptr_t vfs_stat(const char *path, char *buf, uintptr_t len);

#endif
