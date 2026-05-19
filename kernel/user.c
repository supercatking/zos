#include <zos/console.h>
#include <zos/memlayout.h>
#include <zos/panic.h>
#include <zos/pmm.h>
#include <zos/riscv.h>
#include <zos/user.h>
#include <zos/vm.h>

extern char __user_init_start[];
extern char __user_init_end[];

static void copy_bytes(void *dst, const void *src, size_t len)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;

    for (size_t i = 0; i < len; i++) {
        d[i] = s[i];
    }
}

static void zero_page(void *page)
{
    uint8_t *p = (uint8_t *)page;

    for (size_t i = 0; i < PAGE_SIZE; i++) {
        p[i] = 0;
    }
}

void user_init(void)
{
    size_t image_size = (size_t)(__user_init_end - __user_init_start);
    void *text_page = pmm_alloc_page();
    void *stack_page = pmm_alloc_page();

    if (text_page == 0 || stack_page == 0) {
        PANIC("user: out of pages");
    }

    if (image_size > PAGE_SIZE) {
        PANIC("user: init image too large");
    }

    zero_page(text_page);
    zero_page(stack_page);
    copy_bytes(text_page, __user_init_start, image_size);

    if (vm_map(vm_kernel_table(), USER_TEXT_BASE, (uintptr_t)text_page,
               PAGE_SIZE, PTE_R | PTE_W | PTE_X | PTE_U) != 0) {
        PANIC("user: map text failed");
    }

    if (vm_map(vm_kernel_table(), USER_STACK_BASE, (uintptr_t)stack_page,
               PAGE_SIZE, PTE_R | PTE_W | PTE_U) != 0) {
        PANIC("user: map stack failed");
    }

    __asm__ volatile("sfence.vma" ::: "memory");
    console_puts("user: init mapped\n");
}

void user_enter(uintptr_t entry, uintptr_t stack_top)
{
    uintptr_t status = r_sstatus();

    status &= ~SSTATUS_SPP;
    status |= SSTATUS_SPIE | SSTATUS_SUM;
    w_sstatus(status);
    w_sepc(entry);

    console_puts("user: entering U-mode\n");
    __asm__ volatile("mv sp, %0\n"
                     "sret"
                     :
                     : "r"(stack_top)
                     : "memory");

    __builtin_unreachable();
}
