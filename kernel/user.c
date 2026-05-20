#include <zos/console.h>
#include <zos/initramfs.h>
#include <zos/memlayout.h>
#include <zos/panic.h>
#include <zos/pmm.h>
#include <zos/riscv.h>
#include <zos/trap.h>
#include <zos/user.h>
#include <zos/vm.h>

extern char _binary_build_user_shell_bin_start[];
extern char _binary_build_user_shell_bin_end[];
extern char _binary_build_user_bin_echo_bin_start[];
extern char _binary_build_user_bin_echo_bin_end[];
extern char _binary_build_user_bin_cat_bin_start[];
extern char _binary_build_user_bin_cat_bin_end[];
extern char _binary_build_user_bin_ls_bin_start[];
extern char _binary_build_user_bin_ls_bin_end[];
extern char _binary_build_user_bin_help_bin_start[];
extern char _binary_build_user_bin_help_bin_end[];

static size_t current_text_pages;
static char current_program[32];

enum proc_state {
    PROC_UNUSED = 0,
    PROC_RUNNING,
    PROC_ZOMBIE,
};

struct proc {
    int pid;
    int ppid;
    int exit_status;
    enum proc_state state;
    struct trap_frame parent_tf;
    uint8_t text[USER_TEXT_PAGES * PAGE_SIZE];
    uint8_t stack[PAGE_SIZE];
};

static struct proc procs[8];
static int current_pid = 1;
static int next_pid = 2;

static int streq(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static void copy_string(char *dst, const char *src, size_t max)
{
    size_t i = 0;

    if (max == 0) {
        return;
    }

    while (i + 1u < max && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static void add_program(const char *path, const char *start, const char *end)
{
    if (initramfs_add_static_file(path, start, (uintptr_t)(end - start)) != 0) {
        PANIC("user: add program failed");
    }
}

void user_register_programs(void)
{
    add_program("/bin/sh", _binary_build_user_shell_bin_start,
                _binary_build_user_shell_bin_end);
    add_program("/bin/echo", _binary_build_user_bin_echo_bin_start,
                _binary_build_user_bin_echo_bin_end);
    add_program("/bin/cat", _binary_build_user_bin_cat_bin_start,
                _binary_build_user_bin_cat_bin_end);
    add_program("/bin/ls", _binary_build_user_bin_ls_bin_start,
                _binary_build_user_bin_ls_bin_end);
    add_program("/bin/help", _binary_build_user_bin_help_bin_start,
                _binary_build_user_bin_help_bin_end);
}

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

static void zero_bytes(void *dst, size_t len)
{
    uint8_t *p = (uint8_t *)dst;

    for (size_t i = 0; i < len; i++) {
        p[i] = 0;
    }
}

static void copy_from_user_image(uint8_t *text, uint8_t *stack)
{
    copy_bytes(text, (const void *)USER_TEXT_BASE, USER_TEXT_PAGES * PAGE_SIZE);
    copy_bytes(stack, (const void *)USER_STACK_BASE, PAGE_SIZE);
}

static void copy_to_user_image(const uint8_t *text, const uint8_t *stack)
{
    (void)stack;
    copy_bytes((void *)USER_TEXT_BASE, text, USER_TEXT_PAGES * PAGE_SIZE);
}

void user_init(void)
{
    size_t image_size = (size_t)(_binary_build_user_shell_bin_end -
                                 _binary_build_user_shell_bin_start);
    void *stack_page = pmm_alloc_page();
    size_t image_pages = (image_size + PAGE_SIZE - 1u) / PAGE_SIZE;

    if (stack_page == 0 || image_pages > USER_TEXT_PAGES) {
        PANIC("user: out of pages");
    }

    zero_page(stack_page);

    for (size_t i = 0; i < USER_TEXT_PAGES; i++) {
        void *text_page = pmm_alloc_page();
        size_t offset = i * PAGE_SIZE;
        size_t remaining = offset < image_size ? image_size - offset : 0;
        size_t copy_len = remaining > PAGE_SIZE ? PAGE_SIZE : remaining;

        if (text_page == 0) {
            PANIC("user: out of text pages");
        }

        zero_page(text_page);
        copy_bytes(text_page, _binary_build_user_shell_bin_start + offset, copy_len);

        if (vm_map(vm_kernel_table(), USER_TEXT_BASE + offset, (uintptr_t)text_page,
                   PAGE_SIZE, PTE_R | PTE_W | PTE_X | PTE_U) != 0) {
            PANIC("user: map text failed");
        }
    }

    current_text_pages = USER_TEXT_PAGES;
    copy_string(current_program, "/bin/sh", sizeof(current_program));
    for (size_t i = 0; i < sizeof(procs) / sizeof(procs[0]); i++) {
        procs[i].state = PROC_UNUSED;
    }

    if (vm_map(vm_kernel_table(), USER_STACK_BASE, (uintptr_t)stack_page,
               PAGE_SIZE, PTE_R | PTE_W | PTE_U) != 0) {
        PANIC("user: map stack failed");
    }

    __asm__ volatile("sfence.vma" ::: "memory");
    console_puts("user: init mapped\n");
}

int user_current_is_shell(void)
{
    return streq(current_program, "/bin/sh");
}

int user_getpid(void)
{
    return current_pid;
}

int user_fork(struct trap_frame *tf)
{
    for (size_t i = 0; i < sizeof(procs) / sizeof(procs[0]); i++) {
        if (procs[i].state == PROC_UNUSED) {
            int pid = next_pid++;

            procs[i].pid = pid;
            procs[i].ppid = current_pid;
            procs[i].exit_status = 0;
            procs[i].state = PROC_RUNNING;
            procs[i].parent_tf = *tf;
            procs[i].parent_tf.a0 = (uintptr_t)pid;
            copy_from_user_image(procs[i].text, procs[i].stack);

            current_pid = pid;
            tf->a0 = 0;
            return 0;
        }
    }

    return -1;
}

int user_exit_process(uintptr_t status, struct trap_frame *tf)
{
    if (current_pid == 1) {
        return 0;
    }

    for (size_t i = 0; i < sizeof(procs) / sizeof(procs[0]); i++) {
        if (procs[i].state == PROC_RUNNING && procs[i].pid == current_pid) {
            procs[i].state = PROC_ZOMBIE;
            procs[i].exit_status = (int)status;
            copy_to_user_image(procs[i].text, procs[i].stack);
            *tf = procs[i].parent_tf;
            current_pid = procs[i].ppid;
            copy_string(current_program, "/bin/sh", sizeof(current_program));
            __asm__ volatile("sfence.vma" ::: "memory");
            return 1;
        }
    }

    return -1;
}

int user_wait(void)
{
    for (size_t i = 0; i < sizeof(procs) / sizeof(procs[0]); i++) {
        if (procs[i].state == PROC_ZOMBIE && procs[i].ppid == current_pid) {
            int pid = procs[i].pid;
            procs[i].state = PROC_UNUSED;
            return pid;
        }
    }

    return -1;
}

int user_exec(const char *path, const char *arg, struct trap_frame *tf)
{
    uintptr_t image_size = 0;
    const char *image = initramfs_data(path, &image_size);
    size_t image_pages = (image_size + PAGE_SIZE - 1u) / PAGE_SIZE;
    uintptr_t arg_base = current_pid == 1 ? USER_ARG_BASE : USER_STACK_BASE + 256u;
    uintptr_t stack_top = current_pid == 1 ? USER_STACK_TOP : USER_STACK_BASE + 2048u;
    char *arg_dst = (char *)arg_base;

    if (image == 0 || tf == 0 || image_pages == 0 ||
        image_pages > current_text_pages) {
        return -1;
    }

    zero_bytes((void *)USER_TEXT_BASE, current_text_pages * PAGE_SIZE);
    copy_bytes((void *)USER_TEXT_BASE, image, image_size);
    copy_string(current_program, path, sizeof(current_program));

    if (arg != 0) {
        copy_string(arg_dst, arg, 256);
        tf->a0 = arg_base;
    } else {
        arg_dst[0] = '\0';
        tf->a0 = arg_base;
    }

    tf->sepc = USER_TEXT_BASE;
    tf->sp = stack_top;
    __asm__ volatile("sfence.vma" ::: "memory");
    return 0;
}

void user_enter(uintptr_t entry, uintptr_t stack_top)
{
    uintptr_t status = r_sstatus();

    status &= ~SSTATUS_SPP;
    status |= SSTATUS_SPIE | SSTATUS_SUM;
    w_sstatus(status);
    w_sepc(entry);

    console_puts("user: entering U-mode\n");
    __asm__ volatile("li a0, 0\n"
                     "mv sp, %0\n"
                     "sret"
                     :
                     : "r"(stack_top)
                     : "memory");

    __builtin_unreachable();
}
