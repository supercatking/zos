#include "../userlib.h"

static int contains(const char *s, const char *needle)
{
    if (*needle == '\0') {
        return 1;
    }

    for (; *s != '\0'; s++) {
        const char *a = s;
        const char *b = needle;
        while (*a != '\0' && *b != '\0' && *a == *b) {
            a++;
            b++;
        }
        if (*b == '\0') {
            return 1;
        }
    }
    return 0;
}

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    char args[96];
    char *pattern = args;
    char *path = 0;
    char buf[128];

    if (line == 0 || line[0] == '\0') {
        u_write("grep: usage grep pattern file\n");
        u_exit(1);
    }

    u_copy_string(args, line, sizeof(args));
    for (char *p = args; *p != '\0'; p++) {
        if (*p == ' ' || *p == '\t') {
            *p++ = '\0';
            while (*p == ' ' || *p == '\t') {
                p++;
            }
            path = p;
            break;
        }
    }

    if (path == 0 || path[0] == '\0') {
        u_write("grep: usage grep pattern file\n");
        u_exit(1);
    }

    int fd = u_open(path);
    if (fd < 0) {
        u_write("grep: not found\n");
        u_exit(1);
    }

    for (;;) {
        long n = u_read(fd, buf, sizeof(buf) - 1u);
        if (n <= 0) {
            break;
        }
        buf[n] = '\0';
        if (contains(buf, pattern)) {
            u_write_buf(buf, (size_t)n);
            if (buf[n - 1] != '\n') {
                u_write("\n");
            }
        }
    }
    u_close(fd);
    u_exit(0);
}
