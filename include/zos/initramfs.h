#ifndef ZOS_INITRAMFS_H
#define ZOS_INITRAMFS_H

#include <zos/types.h>

void initramfs_init(void);
int initramfs_open(const char *path);
uintptr_t initramfs_read(int fd, char *buf, uintptr_t len);
int initramfs_close(int fd);
const char *initramfs_list(void);

#endif
