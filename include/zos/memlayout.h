#ifndef ZOS_MEMLAYOUT_H
#define ZOS_MEMLAYOUT_H

#include <zos/types.h>

#define PAGE_SIZE 4096u
#define PAGE_SHIFT 12u

#define KERNEL_BASE 0x80200000u
#define PHYS_MEM_BASE 0x80000000u
#define PHYS_MEM_TOP 0x88000000u

#define UART0_BASE 0x10000000u

static inline uintptr_t page_align_down(uintptr_t value)
{
    return value & ~(uintptr_t)(PAGE_SIZE - 1u);
}

static inline uintptr_t page_align_up(uintptr_t value)
{
    return (value + PAGE_SIZE - 1u) & ~(uintptr_t)(PAGE_SIZE - 1u);
}

static inline int page_aligned(uintptr_t value)
{
    return (value & (PAGE_SIZE - 1u)) == 0;
}

#endif
