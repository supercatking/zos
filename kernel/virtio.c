#include <zos/console.h>
#include <zos/memlayout.h>
#include <zos/pmm.h>
#include <zos/types.h>
#include <zos/virtio.h>

#define VIRTIO_MMIO_MAGIC_VALUE 0x74726976u
#define VIRTIO_DEVICE_BLOCK 2u
#define VIRTIO_BLK_SECTOR_SIZE 512u
#define VIRTIO_QUEUE_SIZE 8u

#define VIRTIO_MMIO_MAGIC 0x000u
#define VIRTIO_MMIO_VERSION 0x004u
#define VIRTIO_MMIO_DEVICE_ID 0x008u
#define VIRTIO_MMIO_VENDOR_ID 0x00cu
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010u
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020u
#define VIRTIO_MMIO_GUEST_PAGE_SIZE 0x028u
#define VIRTIO_MMIO_QUEUE_SEL 0x030u
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034u
#define VIRTIO_MMIO_QUEUE_NUM 0x038u
#define VIRTIO_MMIO_QUEUE_ALIGN 0x03cu
#define VIRTIO_MMIO_QUEUE_PFN 0x040u
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x050u
#define VIRTIO_MMIO_INTERRUPT_STATUS 0x060u
#define VIRTIO_MMIO_INTERRUPT_ACK 0x064u
#define VIRTIO_MMIO_STATUS 0x070u

#define VIRTIO_STATUS_ACKNOWLEDGE 1u
#define VIRTIO_STATUS_DRIVER 2u
#define VIRTIO_STATUS_DRIVER_OK 4u
#define VIRTIO_STATUS_FAILED 128u

#define VIRTQ_DESC_F_NEXT 1u
#define VIRTQ_DESC_F_WRITE 2u

#define VIRTIO_BLK_T_IN 0u

struct virtq_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
};

struct virtq_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[VIRTIO_QUEUE_SIZE];
};

struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
};

struct virtq_used {
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[VIRTIO_QUEUE_SIZE];
};

struct virtio_blk_req {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
};

static uintptr_t blk_base;
static uint32_t blk_version;
static uint8_t blk_queue[PAGE_SIZE * 2u] __attribute__((aligned(PAGE_SIZE)));
static int blk_queue_ready;
static volatile struct virtq_desc *blk_desc;
static volatile struct virtq_avail *blk_avail;
static volatile struct virtq_used *blk_used;
static struct virtio_blk_req blk_req;
static uint8_t blk_status;
static uint16_t blk_last_used;

static uint32_t mmio_read32(uintptr_t addr)
{
    return *(volatile uint32_t *)addr;
}

static void mmio_write32(uintptr_t addr, uint32_t value)
{
    *(volatile uint32_t *)addr = value;
}

static void memzero(void *ptr, uintptr_t len)
{
    uint8_t *p = (uint8_t *)ptr;

    for (uintptr_t i = 0; i < len; i++) {
        p[i] = 0;
    }
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
            blk_version = version;
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

static int virtio_blk_init_queue(void)
{
    uint32_t queue_max;

    if (blk_base == 0) {
        return -1;
    }
    if (blk_queue_ready) {
        return 0;
    }
    if (blk_version != 1u) {
        console_puts("virtio-blk: unsupported mmio version\n");
        return -1;
    }

    memzero(blk_queue, sizeof(blk_queue));

    blk_desc = (volatile struct virtq_desc *)blk_queue;
    blk_avail = (volatile struct virtq_avail *)((uintptr_t)blk_queue + 128u);
    blk_used = (volatile struct virtq_used *)((uintptr_t)blk_queue + PAGE_SIZE);
    blk_last_used = 0;

    mmio_write32(blk_base + VIRTIO_MMIO_STATUS, 0);
    mmio_write32(blk_base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);
    mmio_write32(blk_base + VIRTIO_MMIO_STATUS,
                 VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);
    (void)mmio_read32(blk_base + VIRTIO_MMIO_DEVICE_FEATURES);
    mmio_write32(blk_base + VIRTIO_MMIO_DRIVER_FEATURES, 0);
    mmio_write32(blk_base + VIRTIO_MMIO_GUEST_PAGE_SIZE, PAGE_SIZE);

    mmio_write32(blk_base + VIRTIO_MMIO_QUEUE_SEL, 0);
    queue_max = mmio_read32(blk_base + VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (queue_max < VIRTIO_QUEUE_SIZE) {
        mmio_write32(blk_base + VIRTIO_MMIO_STATUS, VIRTIO_STATUS_FAILED);
        return -1;
    }
    mmio_write32(blk_base + VIRTIO_MMIO_QUEUE_NUM, VIRTIO_QUEUE_SIZE);
    mmio_write32(blk_base + VIRTIO_MMIO_QUEUE_ALIGN, PAGE_SIZE);
    mmio_write32(blk_base + VIRTIO_MMIO_QUEUE_PFN,
                 (uint32_t)((uintptr_t)blk_queue >> PAGE_SHIFT));
    mmio_write32(blk_base + VIRTIO_MMIO_STATUS,
                 VIRTIO_STATUS_ACKNOWLEDGE |
                     VIRTIO_STATUS_DRIVER |
                     VIRTIO_STATUS_DRIVER_OK);

    blk_queue_ready = 1;
    console_puts("virtio-blk: queue ready\n");
    return 0;
}

int virtio_blk_read_sector(uint64_t sector, void *buf)
{
    uint16_t avail_idx;
    uint16_t used_idx;

    if (buf == 0 || virtio_blk_init_queue() != 0) {
        return -1;
    }

    blk_req.type = VIRTIO_BLK_T_IN;
    blk_req.reserved = 0;
    blk_req.sector = sector;
    blk_status = 0xffu;

    blk_desc[0].addr = (uint64_t)(uintptr_t)&blk_req;
    blk_desc[0].len = sizeof(blk_req);
    blk_desc[0].flags = VIRTQ_DESC_F_NEXT;
    blk_desc[0].next = 1;

    blk_desc[1].addr = (uint64_t)(uintptr_t)buf;
    blk_desc[1].len = VIRTIO_BLK_SECTOR_SIZE;
    blk_desc[1].flags = VIRTQ_DESC_F_NEXT | VIRTQ_DESC_F_WRITE;
    blk_desc[1].next = 2;

    blk_desc[2].addr = (uint64_t)(uintptr_t)&blk_status;
    blk_desc[2].len = sizeof(blk_status);
    blk_desc[2].flags = VIRTQ_DESC_F_WRITE;
    blk_desc[2].next = 0;

    avail_idx = blk_avail->idx;
    blk_avail->ring[avail_idx % VIRTIO_QUEUE_SIZE] = 0;
    __sync_synchronize();
    blk_avail->idx = avail_idx + 1u;
    __sync_synchronize();
    mmio_write32(blk_base + VIRTIO_MMIO_QUEUE_NOTIFY, 0);

    do {
        used_idx = blk_used->idx;
    } while (used_idx == blk_last_used);

    blk_last_used = used_idx;
    if (mmio_read32(blk_base + VIRTIO_MMIO_INTERRUPT_STATUS) != 0) {
        mmio_write32(blk_base + VIRTIO_MMIO_INTERRUPT_ACK,
                     mmio_read32(blk_base + VIRTIO_MMIO_INTERRUPT_STATUS));
    }

    return blk_status == 0 ? 0 : -1;
}
