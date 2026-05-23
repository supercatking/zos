#include "../userlib.h"

static int split_two(char *line, char **a, char **b)
{
    char *p = line;

    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (*p == '\0') {
        return -1;
    }
    *a = p;
    while (*p != '\0' && *p != ' ' && *p != '\t') {
        p++;
    }
    if (*p == '\0') {
        return -1;
    }
    *p++ = '\0';
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (*p == '\0') {
        return -1;
    }
    *b = p;
    while (*p != '\0' && *p != ' ' && *p != '\t') {
        p++;
    }
    *p = '\0';
    return 0;
}

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    char args[96];
    char buf[128];
    char *src_path;
    char *dst_path;
    int src;
    int dst;

    if (line == 0) {
        u_write("cp: missing operand\n");
        u_exit(1);
    }

    u_copy_string(args, line, sizeof(args));
    if (split_two(args, &src_path, &dst_path) != 0) {
        u_write("cp: missing operand\n");
        u_exit(1);
    }

    src = u_open(src_path);
    if (src < 0 || u_create(dst_path) != 0) {
        u_write("cp: failed\n");
        u_exit(1);
    }
    dst = u_open(dst_path);
    if (dst < 0) {
        u_close(src);
        u_write("cp: failed\n");
        u_exit(1);
    }

    for (;;) {
        long n = u_read(src, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        if (u_write_fd(dst, buf, (size_t)n) != n) {
            u_close(src);
            u_close(dst);
            u_write("cp: failed\n");
            u_exit(1);
        }
    }

    u_close(src);
    u_close(dst);
    u_exit(0);
}
