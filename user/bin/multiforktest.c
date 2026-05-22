#include "../userlib.h"

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    (void)line;
    int children = 0;

    u_write("multifork: parent start\n");
    for (int i = 0; i < 3; i++) {
        long pid = u_fork();
        if (pid == 0) {
            u_write("multifork: child start\n");
            u_exit(0);
        }
        if (pid < 0) {
            u_write("multifork: fork failed\n");
            u_exit(1);
        }
        children++;
    }

    for (int i = 0; i < children; i++) {
        if (u_wait() <= 0) {
            u_write("multifork: wait failed\n");
            u_exit(1);
        }
    }

    u_write("multifork: wait reaped 3\n");
    u_write("multifork: ok\n");
    u_exit(0);
}
