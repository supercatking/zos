#include "../userlib.h"

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    const char *path = line != 0 && line[0] != '\0' ? line : "/README";
    char buf[96];
    int fd = u_open(path);

    if (fd < 0) {
        u_write("cat: not found\n");
        u_exit(1);
    }

    for (;;) {
        long n = u_read(fd, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        u_write_buf(buf, (size_t)n);
    }
    u_close(fd);
    u_exit(0);
}
