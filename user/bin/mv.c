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
    char buf[96];
    char *old_path;
    char *new_path;

    if (line == 0) {
        u_write("mv: missing operand\n");
        u_exit(1);
    }

    u_copy_string(buf, line, sizeof(buf));
    if (split_two(buf, &old_path, &new_path) != 0) {
        u_write("mv: missing operand\n");
        u_exit(1);
    }
    if (u_rename(old_path, new_path) != 0) {
        u_write("mv: failed\n");
        u_exit(1);
    }
    u_exit(0);
}
