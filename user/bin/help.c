#include "../userlib.h"

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    (void)line;
    u_write("commands: help echo ls cat touch rm mkdir rmdir stat cp mv ps sleep kill uptime mem uname pwd cd env which history grep wc true false clear reboot\n");
    u_exit(0);
}
