#include <zos/console.h>

#define UART0_BASE 0x10000000u
#define UART_THR   0x00u
#define UART_RBR   0x00u
#define UART_LSR   0x05u
#define UART_LSR_DR 0x01u
#define UART_LSR_THRE 0x20u

static volatile unsigned char *const uart0 = (volatile unsigned char *)UART0_BASE;

void console_init(void)
{
}

void console_putchar(char ch)
{
    if (ch == '\n') {
        console_putchar('\r');
    }

    while ((uart0[UART_LSR] & UART_LSR_THRE) == 0) {
    }
    uart0[UART_THR] = (unsigned char)ch;
}

char console_getchar(void)
{
    while ((uart0[UART_LSR] & UART_LSR_DR) == 0) {
    }
    return (char)uart0[UART_RBR];
}

void console_puts(const char *text)
{
    while (*text != '\0') {
        console_putchar(*text);
        text++;
    }
}

void console_put_hex(uintptr_t value)
{
    static const char digits[] = "0123456789abcdef";

    console_puts("0x");
    for (int shift = (int)(sizeof(uintptr_t) * 8u) - 4; shift >= 0; shift -= 4) {
        console_putchar(digits[(value >> (unsigned)shift) & 0xfu]);
    }
}
