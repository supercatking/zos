#include "../userlib.h"

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    (void)line;
    u_exit(0);
}
