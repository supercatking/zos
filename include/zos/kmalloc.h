#ifndef ZOS_KMALLOC_H
#define ZOS_KMALLOC_H

#include <zos/types.h>

void kmalloc_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);
void kmalloc_self_test(void);

#endif
