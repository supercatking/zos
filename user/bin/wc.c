#include "../userlib.h"

static int is_space(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    const char *path = line != 0 && line[0] != '\0' ? line : "/README";
    char buf[128];
    uintptr_t bytes = 0;
    uintptr_t lines = 0;
    uintptr_t words = 0;
    int in_word = 0;
    int fd = u_open(path);

    if (fd < 0) {
        u_write("wc: not found\n");
        u_exit(1);
    }

    for (;;) {
        long n = u_read(fd, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        bytes += (uintptr_t)n;
        for (long i = 0; i < n; i++) {
            if (buf[i] == '\n') {
                lines++;
            }
            if (is_space(buf[i])) {
                in_word = 0;
            } else if (!in_word) {
                words++;
                in_word = 1;
            }
        }
    }
    u_close(fd);

    u_write("lines=");
    u_put_uint(lines);
    u_write(" words=");
    u_put_uint(words);
    u_write(" bytes=");
    u_put_uint(bytes);
    u_write("\n");
    u_exit(0);
}
