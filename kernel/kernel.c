#include <zos/assert.h>
#include <zos/block.h>
#include <zos/console.h>
#include <zos/elf.h>
#include <zos/initramfs.h>
#include <zos/pmm.h>
#include <zos/riscv.h>
#include <zos/thread.h>
#include <zos/timer.h>
#include <zos/trap.h>
#include <zos/types.h>
#include <zos/user.h>
#include <zos/vfs.h>
#include <zos/virtio.h>
#include <zos/vm.h>

extern char __kernel_start[];
extern char __text_start[];
extern char __text_end[];
extern char __rodata_start[];
extern char __rodata_end[];
extern char __data_start[];
extern char __data_end[];
extern char __bss_start[];
extern char __bss_end[];
extern char __kernel_end[];

static void print_addr(const char *name, const void *addr)
{
    console_puts("  ");
    console_puts(name);
    console_puts(" = ");
    console_put_hex((uintptr_t)addr);
    console_puts("\n");
}

static void worker_thread(void *arg)
{
    const char *name = (const char *)arg;

    for (int i = 0; i < 4; i++) {
        console_puts("thread: ");
        console_puts(name);
        console_puts(" iteration ");
        console_put_hex((uintptr_t)i);
        console_puts("\n");
        thread_yield();
    }
}

void kernel_main(uintptr_t hart_id, uintptr_t dtb)
{
    console_init();
    console_puts("\nZOS booting...\n");
    console_puts("RISC-V32 OpenSBI supervisor kernel\n");
    console_puts("hart id = ");
    console_put_hex(hart_id);
    console_puts("\n");
    console_puts("dtb     = ");
    console_put_hex(dtb);
    console_puts("\n");

    console_puts("kernel layout:\n");
    print_addr("__kernel_start", __kernel_start);
    print_addr("__text_start", __text_start);
    print_addr("__text_end", __text_end);
    print_addr("__rodata_start", __rodata_start);
    print_addr("__rodata_end", __rodata_end);
    print_addr("__data_start", __data_start);
    print_addr("__data_end", __data_end);
    print_addr("__bss_start", __bss_start);
    print_addr("__bss_end", __bss_end);
    print_addr("__kernel_end", __kernel_end);

    pmm_init();
    pmm_self_test();
    vm_init();
    vm_enable_kernel_paging();
    console_puts("vm: paging enabled\n");
    initramfs_init();
    user_register_programs();
    vfs_init();
    virtio_mmio_probe();
    block_cache_init();
    elf32_self_test();

    trap_init();
    console_puts("trap: initialized\n");

    timer_init();
    csr_set(sie, SIE_STIE);
    intr_on();
    console_puts("timer: initialized\n");

    thread_init();
    ASSERT(thread_create("alpha", worker_thread, "alpha") >= 0);
    ASSERT(thread_create("beta", worker_thread, "beta") >= 0);
    thread_yield();
    console_puts("thread: cooperative smoke complete\n");

    user_init();
    user_enter(user_init_entry(), USER_STACK_TOP);
}
