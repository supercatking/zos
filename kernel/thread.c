#include <zos/console.h>
#include <zos/panic.h>
#include <zos/thread.h>
#include <zos/types.h>

#define MAX_THREADS 4u
#define THREAD_STACK_SIZE 4096u

enum thread_state {
    THREAD_UNUSED = 0,
    THREAD_RUNNABLE,
    THREAD_RUNNING,
};

struct context {
    uintptr_t ra;
    uintptr_t sp;
    uintptr_t s0;
    uintptr_t s1;
    uintptr_t s2;
    uintptr_t s3;
    uintptr_t s4;
    uintptr_t s5;
    uintptr_t s6;
    uintptr_t s7;
    uintptr_t s8;
    uintptr_t s9;
    uintptr_t s10;
    uintptr_t s11;
};

struct thread {
    struct context ctx;
    enum thread_state state;
    const char *name;
    thread_func_t func;
    void *arg;
    uint8_t stack[THREAD_STACK_SIZE];
};

extern void thread_switch(struct context *old, struct context *new);

static struct thread threads[MAX_THREADS];
static struct context scheduler_ctx;
static struct thread *current;

static void thread_trampoline(void)
{
    current->func(current->arg);
    console_puts("thread exited: ");
    console_puts(current->name);
    console_puts("\n");
    current->state = THREAD_UNUSED;
    thread_yield();
    PANIC("thread returned after exit");
}

void thread_init(void)
{
    for (size_t i = 0; i < MAX_THREADS; i++) {
        threads[i].state = THREAD_UNUSED;
    }
    current = 0;
}

int thread_create(const char *name, thread_func_t func, void *arg)
{
    for (size_t i = 0; i < MAX_THREADS; i++) {
        if (threads[i].state == THREAD_UNUSED) {
            uintptr_t sp = (uintptr_t)&threads[i].stack[THREAD_STACK_SIZE];
            sp &= ~(uintptr_t)0xf;

            threads[i].ctx.ra = (uintptr_t)thread_trampoline;
            threads[i].ctx.sp = sp;
            threads[i].state = THREAD_RUNNABLE;
            threads[i].name = name;
            threads[i].func = func;
            threads[i].arg = arg;
            return (int)i;
        }
    }

    return -1;
}

void thread_yield(void)
{
    if (current == 0) {
        return;
    }

    if (current->state == THREAD_RUNNING) {
        current->state = THREAD_RUNNABLE;
    }
    thread_switch(&current->ctx, &scheduler_ctx);
}

void thread_start_scheduler(void)
{
    console_puts("thread: scheduler start\n");

    for (;;) {
        int ran = 0;

        for (size_t i = 0; i < MAX_THREADS; i++) {
            if (threads[i].state != THREAD_RUNNABLE) {
                continue;
            }

            ran = 1;
            current = &threads[i];
            current->state = THREAD_RUNNING;
            thread_switch(&scheduler_ctx, &current->ctx);
        }

        if (!ran) {
            __asm__ volatile("wfi");
        }
    }
}
