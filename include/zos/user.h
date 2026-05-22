#ifndef ZOS_USER_H
#define ZOS_USER_H

#include <zos/types.h>

#define USER_TEXT_BASE 0x00400000u
#define USER_STACK_BASE 0x00410000u
#define USER_STACK_TOP  0x00411000u
#define USER_ARG_BASE   (USER_STACK_TOP - 512u)
#define USER_TEXT_PAGES ((USER_STACK_BASE - USER_TEXT_BASE) / 4096u)

struct trap_frame;

void user_init(void);
void user_register_programs(void);
int user_exec(const char *path, const char *arg, struct trap_frame *tf);
int user_current_is_shell(void);
int user_fork(struct trap_frame *tf);
int user_wait(struct trap_frame *tf);
int user_getpid(void);
uintptr_t user_procinfo(char *buf, uintptr_t len);
int user_exit_process(uintptr_t status, struct trap_frame *tf);
void user_timer_tick(struct trap_frame *tf);
void user_enter(uintptr_t entry, uintptr_t stack_top) __attribute__((noreturn));

#endif
