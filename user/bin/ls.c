#include "../userlib.h"

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    const char *path = line != 0 && line[0] != '\0' ? line : "/";
    char buf[160];
    long n = u_list(path, buf, sizeof(buf));

    if (n > 0) {
        u_write_buf(buf, (size_t)n);
    }
    u_exit(0);
}
