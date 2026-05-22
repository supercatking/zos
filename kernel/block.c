#include <zos/block.h>
#include <zos/console.h>
#include <zos/types.h>
#include <zos/virtio.h>

static uint8_t cache_data[BLOCK_SECTOR_SIZE];
static uint64_t cache_sector;
static int cache_valid;

static void copy_bytes(void *dst, const void *src, uintptr_t len)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;

    for (uintptr_t i = 0; i < len; i++) {
        d[i] = s[i];
    }
}

void block_cache_init(void)
{
    cache_valid = 0;

    if (!virtio_blk_found()) {
        console_puts("block: no device\n");
        return;
    }

    if (block_read_sector(0, cache_data) == 0) {
        console_puts("block: cache ready\n");
    } else {
        console_puts("block: cache init failed\n");
    }
}

int block_read_sector(uint64_t sector, void *buf)
{
    if (buf == 0 || !virtio_blk_found()) {
        return -1;
    }

    if (!cache_valid || cache_sector != sector) {
        if (virtio_blk_read_sector(sector, cache_data) != 0) {
            return -1;
        }
        cache_sector = sector;
        cache_valid = 1;
    }

    copy_bytes(buf, cache_data, BLOCK_SECTOR_SIZE);
    return 0;
}

int block_write_sector(uint64_t sector, const void *buf)
{
    if (buf == 0 || !virtio_blk_found()) {
        return -1;
    }

    copy_bytes(cache_data, buf, BLOCK_SECTOR_SIZE);
    cache_sector = sector;
    cache_valid = 1;
    return virtio_blk_write_sector(sector, cache_data);
}
