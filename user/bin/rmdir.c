#include "../userlib.h"

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    if (line == 0 || line[0] == '\0') {
        u_write("rmdir: missing dir\n");
        u_exit(1);
    }
    if (u_unlink(line) != 0) {
        u_write("rmdir: failed\n");
        u_exit(1);
    }
    u_exit(0);
}
