#ifndef ZOS_INITRAMFS_H
#define ZOS_INITRAMFS_H

#include <zos/types.h>

void initramfs_init(void);
int initramfs_add_static_file(const char *path, const char *data, uintptr_t len);
int initramfs_open(const char *path);
int initramfs_create(const char *path);
int initramfs_mkdir(const char *path);
uintptr_t initramfs_read(int fd, char *buf, uintptr_t len);
uintptr_t initramfs_write(int fd, const char *buf, uintptr_t len);
int initramfs_close(int fd);
uintptr_t initramfs_list(const char *path, char *buf, uintptr_t len);
const char *initramfs_data(const char *path, uintptr_t *len);
int initramfs_unlink(const char *path);
int initramfs_rename(const char *old_path, const char *new_path);
uintptr_t initramfs_stat(const char *path, char *buf, uintptr_t len);

#endif
