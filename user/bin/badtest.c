#include "../userlib.h"

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    (void)line;
    volatile unsigned int *bad = (volatile unsigned int *)0x0;

    *bad = 0x12345678u;
    u_write("badtest: survived unexpected fault\n");
    u_exit(1);
}
