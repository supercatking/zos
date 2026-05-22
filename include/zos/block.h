#ifndef ZOS_BLOCK_H
#define ZOS_BLOCK_H

#include <zos/types.h>

#define BLOCK_SECTOR_SIZE 512u

void block_cache_init(void);
int block_read_sector(uint64_t sector, void *buf);
int block_write_sector(uint64_t sector, const void *buf);

#endif
