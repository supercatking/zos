#ifndef ZOS_CONSOLE_H
#define ZOS_CONSOLE_H

#include <zos/types.h>

void console_init(void);
void console_putchar(char ch);
char console_getchar(void);
void console_poll_input(void);
int console_read_char(char *ch);
void console_puts(const char *text);
void console_put_hex(uintptr_t value);

#endif
