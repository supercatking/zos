#ifndef ZOS_SYSCALL_H
#define ZOS_SYSCALL_H

#include <zos/trap.h>

#define SYS_WRITE 1u
#define SYS_EXIT 2u
#define SYS_READ 3u
#define SYS_OPEN 4u
#define SYS_CLOSE 5u
#define SYS_SLEEP 6u
#define SYS_KILL 7u
#define SYS_CREATE 8u
#define SYS_LIST 9u
#define SYS_UNLINK 10u
#define SYS_STAT 11u
#define SYS_MKDIR 12u
#define SYS_RENAME 13u
#define SYS_UPTIME 14u
#define SYS_MEMINFO 15u

void syscall_handle(struct trap_frame *tf);

#endif
