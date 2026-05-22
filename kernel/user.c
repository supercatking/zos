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
extern char _binary_build_user_bin_forktest_bin_start[];
extern char _binary_build_user_bin_forktest_bin_end[];
extern char _binary_build_user_bin_vmtest_bin_start[];
extern char _binary_build_user_bin_vmtest_bin_end[];

static size_t current_text_pages;
static char current_program[32];

enum proc_state {
    PROC_UNUSED = 0,
    PROC_RUNNABLE,
    PROC_RUNNING,
    PROC_SLEEPING,
    PROC_BLOCKED,
    PROC_ZOMBIE,
};

struct proc {
    int pid;
    int ppid;
    int exit_status;
    enum proc_state state;
    pagetable_t pagetable;
    void *text_pages[USER_TEXT_PAGES];
    void *stack_page;
    uint8_t kernel_stack[PAGE_SIZE];
    struct trap_frame tf;
    struct trap_frame parent_tf;
    int waiting;
};

static struct proc procs[8];
static struct proc *current_proc;
static int current_pid = 1;
static int next_pid = 2;

static const char *proc_state_name(enum proc_state state)
{
    switch (state) {
    case PROC_RUNNABLE:
        return "runnable";
    case PROC_RUNNING:
        return "running";
    case PROC_SLEEPING:
        return "sleeping";
    case PROC_BLOCKED:
        return "blocked";
    case PROC_ZOMBIE:
        return "zombie";
    case PROC_UNUSED:
    default:
        return "unused";
    }
}

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

static uintptr_t append_char(char *buf, uintptr_t out, uintptr_t len, char ch)
{
    if (out < len) {
        buf[out++] = ch;
    }
    return out;
}

static uintptr_t append_str(char *buf, uintptr_t out, uintptr_t len, const char *s)
{
    for (uintptr_t i = 0; s[i] != '\0'; i++) {
        out = append_char(buf, out, len, s[i]);
    }
    return out;
}

static uintptr_t append_uint(char *buf, uintptr_t out, uintptr_t len, uintptr_t value)
{
    char tmp[10];
    uintptr_t n = 0;

    if (value == 0) {
        return append_char(buf, out, len, '0');
    }

    while (value != 0 && n < sizeof(tmp)) {
        tmp[n++] = (char)('0' + value % 10u);
        value /= 10u;
    }
    while (n != 0) {
        out = append_char(buf, out, len, tmp[--n]);
    }
    return out;
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
    add_program("/bin/forktest", _binary_build_user_bin_forktest_bin_start,
                _binary_build_user_bin_forktest_bin_end);
    add_program("/bin/vmtest", _binary_build_user_bin_vmtest_bin_start,
                _binary_build_user_bin_vmtest_bin_end);
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

static struct proc *proc_by_pid(int pid)
{
    for (size_t i = 0; i < sizeof(procs) / sizeof(procs[0]); i++) {
        if (procs[i].state != PROC_UNUSED && procs[i].pid == pid) {
            return &procs[i];
        }
    }
    return 0;
}

static int proc_alloc_address_space(struct proc *proc)
{
    proc->pagetable = vm_create_user_table();
    if (proc->pagetable == 0) {
        return -1;
    }

    for (size_t i = 0; i < USER_TEXT_PAGES; i++) {
        proc->text_pages[i] = pmm_alloc_page();
        if (proc->text_pages[i] == 0) {
            return -1;
        }
        zero_page(proc->text_pages[i]);
        if (vm_map(proc->pagetable, USER_TEXT_BASE + i * PAGE_SIZE,
                   (uintptr_t)proc->text_pages[i], PAGE_SIZE,
                   PTE_R | PTE_W | PTE_X | PTE_U) != 0) {
            return -1;
        }
    }

    proc->stack_page = pmm_alloc_page();
    if (proc->stack_page == 0) {
        return -1;
    }
    zero_page(proc->stack_page);
    if (vm_map(proc->pagetable, USER_STACK_BASE, (uintptr_t)proc->stack_page,
               PAGE_SIZE, PTE_R | PTE_W | PTE_U) != 0) {
        return -1;
    }

    return 0;
}

static void proc_copy_user_image(struct proc *dst, struct proc *src)
{
    for (size_t i = 0; i < USER_TEXT_PAGES; i++) {
        copy_bytes(dst->text_pages[i], src->text_pages[i], PAGE_SIZE);
    }
    copy_bytes(dst->stack_page, src->stack_page, PAGE_SIZE);
}

static void proc_load_image(struct proc *proc, const char *image, size_t image_size)
{
    for (size_t i = 0; i < USER_TEXT_PAGES; i++) {
        size_t offset = i * PAGE_SIZE;
        size_t remaining = offset < image_size ? image_size - offset : 0;
        size_t copy_len = remaining > PAGE_SIZE ? PAGE_SIZE : remaining;

        zero_page(proc->text_pages[i]);
        if (copy_len != 0) {
            copy_bytes(proc->text_pages[i], image + offset, copy_len);
        }
    }
}

void user_init(void)
{
    size_t image_size = (size_t)(_binary_build_user_shell_bin_end -
                                 _binary_build_user_shell_bin_start);
    size_t image_pages = (image_size + PAGE_SIZE - 1u) / PAGE_SIZE;
    struct proc *init = &procs[0];

    if (image_pages > USER_TEXT_PAGES) {
        PANIC("user: out of pages");
    }

    current_text_pages = USER_TEXT_PAGES;
    copy_string(current_program, "/bin/sh", sizeof(current_program));
    for (size_t i = 0; i < sizeof(procs) / sizeof(procs[0]); i++) {
        procs[i].state = PROC_UNUSED;
    }

    init->pid = 1;
    init->ppid = 0;
    init->exit_status = 0;
    init->state = PROC_RUNNING;
    init->waiting = 0;
    if (proc_alloc_address_space(init) != 0) {
        PANIC("user: init address space failed");
    }
    proc_load_image(init, _binary_build_user_shell_bin_start, image_size);
    current_proc = init;
    current_pid = init->pid;

    __asm__ volatile("sfence.vma" ::: "memory");
    console_puts("user: init pid=1 address space ready\n");
}

int user_current_is_shell(void)
{
    return streq(current_program, "/bin/sh");
}

int user_getpid(void)
{
    return current_pid;
}

uintptr_t user_procinfo(char *buf, uintptr_t len)
{
    uintptr_t out = 0;

    for (size_t i = 0; i < sizeof(procs) / sizeof(procs[0]); i++) {
        if (procs[i].state == PROC_UNUSED) {
            continue;
        }

        out = append_str(buf, out, len, "pid: ");
        out = append_uint(buf, out, len, (uintptr_t)procs[i].pid);
        out = append_str(buf, out, len, " ppid: ");
        out = append_uint(buf, out, len, (uintptr_t)procs[i].ppid);
        out = append_str(buf, out, len, " state: ");
        out = append_str(buf, out, len, proc_state_name(procs[i].state));
        out = append_str(buf, out, len, procs[i].pid == 1 ? " name: sh\n" : " name: child\n");
    }

    return out;
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
            procs[i].waiting = 0;
            if (proc_alloc_address_space(&procs[i]) != 0) {
                procs[i].state = PROC_UNUSED;
                return -1;
            }
            procs[i].parent_tf = *tf;
            procs[i].parent_tf.a0 = (uintptr_t)pid;
            proc_copy_user_image(&procs[i], current_proc);

            current_proc = &procs[i];
            current_pid = pid;
            vm_switch(current_proc->pagetable);
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
            struct proc *parent = proc_by_pid(procs[i].ppid);

            if (parent == 0) {
                return -1;
            }

            procs[i].state = PROC_ZOMBIE;
            procs[i].exit_status = (int)status;
            *tf = procs[i].parent_tf;
            current_proc = parent;
            current_pid = procs[i].ppid;
            vm_switch(current_proc->pagetable);
            copy_string(current_program, "/bin/sh", sizeof(current_program));
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
    uintptr_t arg_base = USER_ARG_BASE;
    uintptr_t stack_top = USER_STACK_TOP;
    char *arg_dst = (char *)arg_base;

    if (image == 0 || tf == 0 || image_pages == 0 ||
        image_pages > current_text_pages) {
        return -1;
    }

    proc_load_image(current_proc, image, image_size);
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
    vm_switch(current_proc->pagetable);
    w_sscratch(trap_kernel_stack_top());

    console_puts("user: entering U-mode\n");
    __asm__ volatile("li a0, 0\n"
                     "mv sp, %0\n"
                     "sret"
                     :
                     : "r"(stack_top)
                     : "memory");

    __builtin_unreachable();
}
