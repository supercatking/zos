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
uintptr_t user_init_entry(void);
int user_exec(const char *path, const char *arg, struct trap_frame *tf);
int user_current_is_shell(void);
int user_fork(struct trap_frame *tf);
int user_wait(struct trap_frame *tf);
int user_getpid(void);
uintptr_t user_procinfo(char *buf, uintptr_t len);
int user_exit_process(uintptr_t status, struct trap_frame *tf);
int user_fd_open(const char *path);
uintptr_t user_fd_read(int fd, char *buf, uintptr_t len);
uintptr_t user_fd_write(int fd, const char *buf, uintptr_t len);
int user_fd_close(int fd);
void user_timer_tick(struct trap_frame *tf);
void user_enter(uintptr_t entry, uintptr_t stack_top) __attribute__((noreturn));

#endif
