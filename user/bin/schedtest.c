#include "../userlib.h"

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    (void)line;
    int children = 0;

    for (int i = 0; i < 3; i++) {
        long pid = u_fork();
        if (pid == 0) {
            u_write("schedtest: child tick\n");
            u_spin(200000u);
            u_write("schedtest: child done\n");
            u_exit(0);
        }
        if (pid < 0) {
            u_write("schedtest: fork failed\n");
            u_exit(1);
        }
        children++;
    }

    for (int i = 0; i < children; i++) {
        if (u_wait() <= 0) {
            u_write("schedtest: wait failed\n");
            u_exit(1);
        }
    }

    u_write("schedtest: wait reaped 3\n");
    u_write("schedtest: ok\n");
    u_exit(0);
}
