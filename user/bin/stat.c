#include "../userlib.h"

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    const char *path = line != 0 && line[0] != '\0' ? line : "/README";
    char buf[96];
    long n = u_stat(path, buf, sizeof(buf));

    if (n < 0) {
        u_write("stat: not found\n");
        u_exit(1);
    }
    u_write_buf(buf, (size_t)n);
    u_exit(0);
}
