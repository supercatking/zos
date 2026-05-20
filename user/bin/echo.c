#include "../userlib.h"

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    if (line != 0 && line[0] != '\0') {
        u_write(line);
    }
    u_write("\n");
    u_exit(0);
}
