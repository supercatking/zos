#include "../userlib.h"

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    const char *path = line != 0 && line[0] != '\0' ? line : 0;
    char buf[128];
    int fd = 0;

    if (path != 0) {
        if (u_copy_file_to_stdout(path) != 0) {
            u_write("cat: not found\n");
            u_exit(1);
        }
    } else {
        for (;;) {
            long n = u_read(fd, buf, sizeof(buf));
            if (n <= 0) {
                break;
            }
            u_write_buf(buf, (size_t)n);
        }
    }
    u_exit(0);
}
