#include <zos/console.h>
#include <zos/kmalloc.h>
#include <zos/memlayout.h>
#include <zos/panic.h>
#include <zos/pmm.h>
#include <zos/types.h>

#define KMALLOC_CLASS_COUNT 5
#define KMALLOC_MAGIC 0x4b4d414cu

struct kmalloc_node {
    struct kmalloc_node *next;
};

struct kmalloc_header {
    uint32_t magic;
    uint32_t class_index;
};

struct kmalloc_class {
    size_t size;
    struct kmalloc_node *free_list;
};

static struct kmalloc_class classes[KMALLOC_CLASS_COUNT] = {
    {16, 0},
    {32, 0},
    {64, 0},
    {128, 0},
    {256, 0},
};

static void zero_bytes(void *ptr, size_t len)
{
    uint8_t *p = (uint8_t *)ptr;

    for (size_t i = 0; i < len; i++) {
        p[i] = 0;
    }
}

static int class_for_size(size_t size)
{
    for (int i = 0; i < KMALLOC_CLASS_COUNT; i++) {
        if (size <= classes[i].size) {
            return i;
        }
    }
    return -1;
}

static int refill_class(int class_index)
{
    uint8_t *page = (uint8_t *)pmm_alloc_page();
    size_t object_size = classes[class_index].size + sizeof(struct kmalloc_header);

    if (page == 0) {
        return -1;
    }

    for (size_t off = 0; off + object_size <= PAGE_SIZE; off += object_size) {
        struct kmalloc_node *node = (struct kmalloc_node *)(page + off);
        node->next = classes[class_index].free_list;
        classes[class_index].free_list = node;
    }
    return 0;
}

void kmalloc_init(void)
{
    for (int i = 0; i < KMALLOC_CLASS_COUNT; i++) {
        classes[i].free_list = 0;
    }
    console_puts("kmalloc: fixed-size allocator ready\n");
}

void *kmalloc(size_t size)
{
    int class_index;
    struct kmalloc_node *node;
    struct kmalloc_header *header;

    if (size == 0) {
        return 0;
    }

    class_index = class_for_size(size);
    if (class_index < 0) {
        return 0;
    }

    if (classes[class_index].free_list == 0 && refill_class(class_index) != 0) {
        return 0;
    }

    node = classes[class_index].free_list;
    classes[class_index].free_list = node->next;
    header = (struct kmalloc_header *)node;
    header->magic = KMALLOC_MAGIC;
    header->class_index = (uint32_t)class_index;
    zero_bytes(header + 1, classes[class_index].size);
    return header + 1;
}

void kfree(void *ptr)
{
    struct kmalloc_header *header;
    struct kmalloc_node *node;
    uint32_t class_index;

    if (ptr == 0) {
        return;
    }

    header = ((struct kmalloc_header *)ptr) - 1;
    if (header->magic != KMALLOC_MAGIC ||
        header->class_index >= KMALLOC_CLASS_COUNT) {
        PANIC("kfree: invalid pointer");
    }

    class_index = header->class_index;
    header->magic = 0;
    node = (struct kmalloc_node *)header;
    node->next = classes[class_index].free_list;
    classes[class_index].free_list = node;
}

void kmalloc_self_test(void)
{
    size_t before = pmm_free_pages();
    void *a = kmalloc(1);
    void *b = kmalloc(33);
    void *c = kmalloc(256);

    if (a == 0 || b == 0 || c == 0 || a == b || b == c || a == c) {
        PANIC("kmalloc: alloc self-test failed");
    }

    if (pmm_free_pages() >= before) {
        PANIC("kmalloc: refill self-test failed");
    }

    kfree(c);
    kfree(b);
    kfree(a);
    void *d = kmalloc(64);
    if (d == 0) {
        PANIC("kmalloc: free self-test failed");
    }
    kfree(d);

    console_puts("kmalloc: self-test passed\n");
}
