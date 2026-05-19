#ifndef ZOS_THREAD_H
#define ZOS_THREAD_H

#include <zos/types.h>

typedef void (*thread_func_t)(void *arg);

void thread_init(void);
int thread_create(const char *name, thread_func_t func, void *arg);
void thread_yield(void);
void thread_start_scheduler(void) __attribute__((noreturn));

#endif
