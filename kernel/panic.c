#include <zos/console.h>
#include <zos/panic.h>

void panic(const char *message, const char *file, int line)
{
    console_puts("\nKERNEL PANIC: ");
    console_puts(message);
    console_puts("\n  at ");
    console_puts(file);
    console_puts(":");

    unsigned int n = (unsigned int)line;
    char buf[11];
    int pos = 0;
    if (n == 0) {
        console_putchar('0');
    } else {
        while (n > 0 && pos < (int)sizeof(buf)) {
            buf[pos++] = (char)('0' + (n % 10u));
            n /= 10u;
        }
        while (pos > 0) {
            console_putchar(buf[--pos]);
        }
    }
    console_puts("\n");

    for (;;) {
        __asm__ volatile("wfi");
    }
}
