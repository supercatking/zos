#ifndef ZOS_SYSCALL_H
#define ZOS_SYSCALL_H

#include <zos/trap.h>

#define SYS_WRITE 1u
#define SYS_EXIT 2u
#define SYS_READ 3u
#define SYS_OPEN 4u
#define SYS_CLOSE 5u

void syscall_handle(struct trap_frame *tf);

#endif
