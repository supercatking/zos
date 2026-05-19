#include <zos/console.h>
#include <zos/memlayout.h>
#include <zos/panic.h>
#include <zos/pmm.h>

struct free_page {
    struct free_page *next;
};

extern char __kernel_end[];

static struct free_page *free_list;
static uintptr_t managed_start;
static uintptr_t managed_end;
static size_t total_pages_count;
static size_t free_pages_count;

static int pmm_page_in_free_list(void *page)
{
    for (struct free_page *node = free_list; node != 0; node = node->next) {
        if ((void *)node == page) {
            return 1;
        }
    }
    return 0;
}

static void pmm_push_page(uintptr_t page)
{
    struct free_page *node = (struct free_page *)page;

    node->next = free_list;
    free_list = node;
    free_pages_count++;
}

void pmm_init(void)
{
    managed_start = page_align_up((uintptr_t)__kernel_end);
    managed_end = PHYS_MEM_TOP;
    free_list = 0;
    total_pages_count = 0;
    free_pages_count = 0;

    for (uintptr_t page = managed_start; page + PAGE_SIZE <= managed_end; page += PAGE_SIZE) {
        pmm_push_page(page);
        total_pages_count++;
    }

    console_puts("pmm: range ");
    console_put_hex(managed_start);
    console_puts("..");
    console_put_hex(managed_end);
    console_puts(" total=");
    console_put_hex((uintptr_t)total_pages_count);
    console_puts(" free=");
    console_put_hex((uintptr_t)free_pages_count);
    console_puts("\n");
}

void *pmm_alloc_page(void)
{
    struct free_page *page = free_list;

    if (page == 0) {
        return 0;
    }

    free_list = page->next;
    free_pages_count--;
    page->next = 0;
    return page;
}

void pmm_free_page(void *page)
{
    uintptr_t addr = (uintptr_t)page;

    if (page == 0) {
        return;
    }

    if (!page_aligned(addr) || addr < managed_start || addr >= managed_end) {
        PANIC("pmm: invalid free");
    }

    if (pmm_page_in_free_list(page)) {
        PANIC("pmm: double free");
    }

    pmm_push_page(addr);
}

size_t pmm_total_pages(void)
{
    return total_pages_count;
}

size_t pmm_free_pages(void)
{
    return free_pages_count;
}

void pmm_self_test(void)
{
    size_t before = pmm_free_pages();
    void *a = pmm_alloc_page();
    void *b = pmm_alloc_page();

    if (a == 0 || b == 0 || a == b) {
        PANIC("pmm: alloc self-test failed");
    }

    pmm_free_page(b);
    pmm_free_page(a);

    if (pmm_free_pages() != before) {
        PANIC("pmm: free self-test failed");
    }

    console_puts("pmm: self-test passed\n");
}
