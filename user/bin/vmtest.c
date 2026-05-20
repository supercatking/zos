#include "../userlib.h"

static int private_value = 7;

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    (void)line;
    long pid = u_fork();

    if (pid == 0) {
        private_value = 42;
        u_write("vmtest: child wrote child-private\n");
        u_exit(0);
    }
    if (pid < 0) {
        u_write("vmtest: fork failed\n");
        u_exit(1);
    }

    (void)u_wait();
    if (private_value == 7) {
        u_write("vmtest: parent still parent-private\n");
        u_write("vmtest: isolation ok\n");
        u_exit(0);
    }

    u_write("vmtest: isolation failed\n");
    u_exit(1);
}
