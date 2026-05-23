#include "../userlib.h"

__attribute__((section(".text.start"), used))
void _start(const char *line)
{
    (void)line;
    char *a = (char *)u_malloc(64);
    char *b = (char *)u_malloc(300);
    char *c;

    if (a == 0 || b == 0) {
        u_write("malloctest: alloc failed\n");
        u_exit(1);
    }

    for (size_t i = 0; i < 64; i++) {
        a[i] = (char)('A' + (i % 26u));
    }
    for (size_t i = 0; i < 300; i++) {
        b[i] = (char)('0' + (i % 10u));
    }
    for (size_t i = 0; i < 64; i++) {
        if (a[i] != (char)('A' + (i % 26u))) {
            u_write("malloctest: overwrite a\n");
            u_exit(1);
        }
    }
    for (size_t i = 0; i < 300; i++) {
        if (b[i] != (char)('0' + (i % 10u))) {
            u_write("malloctest: overwrite b\n");
            u_exit(1);
        }
    }

    u_free(a);
    c = (char *)u_malloc(32);
    if (c == 0 || c != a) {
        u_write("malloctest: reuse failed\n");
        u_exit(1);
    }

    u_write("malloctest: ok\n");
    u_exit(0);
}
