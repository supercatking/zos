#ifndef ZOS_USER_H
#define ZOS_USER_H

#include <zos/types.h>

#define USER_TEXT_BASE 0x00400000u
#define USER_STACK_BASE 0x00410000u
#define USER_STACK_TOP  0x00411000u

void user_init(void);
void user_register_programs(void);
void user_enter(uintptr_t entry, uintptr_t stack_top) __attribute__((noreturn));

#endif
