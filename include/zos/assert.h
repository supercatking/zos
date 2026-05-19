#ifndef ZOS_ASSERT_H
#define ZOS_ASSERT_H

#include <zos/panic.h>

#define ASSERT(expr) \
    do { \
        if (!(expr)) { \
            PANIC("assertion failed: " #expr); \
        } \
    } while (0)

#endif
