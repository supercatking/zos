#include "../userlib.h"

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    const char *path = line != 0 && line[0] != '\0' ? line : "/README";

    if (u_copy_file_to_stdout(path) != 0) {
        u_write("cat: not found\n");
        u_exit(1);
    }
    u_exit(0);
}
