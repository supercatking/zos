#ifndef ZOS_PANIC_H
#define ZOS_PANIC_H

void panic(const char *message, const char *file, int line) __attribute__((noreturn));

#define PANIC(message) panic((message), __FILE__, __LINE__)

#endif
