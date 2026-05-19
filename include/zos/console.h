#ifndef ZOS_CONSOLE_H
#define ZOS_CONSOLE_H

#include <zos/types.h>

void console_init(void);
void console_putchar(char ch);
void console_puts(const char *text);
void console_put_hex(uintptr_t value);

#endif
