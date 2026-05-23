#include "../userlib.h"

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    if (line == 0 || line[0] == '\0') {
        u_write("mkdir: missing dir\n");
        u_exit(1);
    }
    if (u_mkdir(line) != 0) {
        u_write("mkdir: failed\n");
        u_exit(1);
    }
    u_exit(0);
}
