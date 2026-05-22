#ifndef ZOS_VIRTIO_H
#define ZOS_VIRTIO_H

#include <zos/types.h>

void virtio_mmio_probe(void);
uintptr_t virtio_blk_found(void);

#endif
