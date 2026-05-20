#include "../userlib.h"

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    (void)line;
    long pid = u_fork();

    if (pid == 0) {
        u_write("forktest: child saw 0\n");
        u_exit(0);
    }
    if (pid < 0) {
        u_write("forktest: fork failed\n");
        u_exit(1);
    }

    u_write("forktest: parent saw child pid\n");
    if (u_wait() > 0) {
        u_write("forktest: wait reaped child\n");
        u_exit(0);
    }

    u_write("forktest: wait failed\n");
    u_exit(1);
}
