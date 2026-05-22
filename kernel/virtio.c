#include <zos/console.h>
#include <zos/memlayout.h>
#include <zos/types.h>
#include <zos/virtio.h>

#define VIRTIO_MMIO_MAGIC_VALUE 0x74726976u
#define VIRTIO_DEVICE_BLOCK 2u

#define VIRTIO_MMIO_MAGIC 0x000u
#define VIRTIO_MMIO_VERSION 0x004u
#define VIRTIO_MMIO_DEVICE_ID 0x008u
#define VIRTIO_MMIO_VENDOR_ID 0x00cu

static uintptr_t blk_base;

static uint32_t mmio_read32(uintptr_t addr)
{
    return *(volatile uint32_t *)addr;
}

static void print_device(uintptr_t base, uint32_t device_id, uint32_t version,
                         uint32_t vendor)
{
    console_puts("virtio-mmio: device base=");
    console_put_hex(base);
    console_puts(" id=");
    console_put_hex(device_id);
    console_puts(" version=");
    console_put_hex(version);
    console_puts(" vendor=");
    console_put_hex(vendor);
    console_puts("\n");
}

void virtio_mmio_probe(void)
{
    blk_base = 0;

    for (uintptr_t i = 0; i < VIRTIO_MMIO_COUNT; i++) {
        uintptr_t base = VIRTIO_MMIO_BASE + i * VIRTIO_MMIO_STRIDE;
        uint32_t magic = mmio_read32(base + VIRTIO_MMIO_MAGIC);
        uint32_t version;
        uint32_t device_id;
        uint32_t vendor;

        if (magic != VIRTIO_MMIO_MAGIC_VALUE) {
            continue;
        }

        version = mmio_read32(base + VIRTIO_MMIO_VERSION);
        device_id = mmio_read32(base + VIRTIO_MMIO_DEVICE_ID);
        vendor = mmio_read32(base + VIRTIO_MMIO_VENDOR_ID);

        if (device_id == 0) {
            continue;
        }

        print_device(base, device_id, version, vendor);
        if (device_id == VIRTIO_DEVICE_BLOCK && blk_base == 0) {
            blk_base = base;
            console_puts("virtio-blk: found\n");
        }
    }

    if (blk_base == 0) {
        console_puts("virtio-blk: not found\n");
    }
}

uintptr_t virtio_blk_found(void)
{
    return blk_base != 0;
}
