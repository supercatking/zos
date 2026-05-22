#ifndef ZOS_VM_H
#define ZOS_VM_H

#include <zos/memlayout.h>
#include <zos/types.h>

#define VM_PAGE_SIZE PAGE_SIZE
#define VM_PAGE_SHIFT PAGE_SHIFT
#define VM_SV32_ENTRIES 1024u

#define VM_UART0_BASE UART0_BASE
#define VM_VIRTIO_MMIO_BASE VIRTIO_MMIO_BASE
#define VM_VIRTIO_MMIO_SIZE VIRTIO_MMIO_SIZE
#define VM_RAM_BASE PHYS_MEM_BASE
#define VM_RAM_END PHYS_MEM_TOP

typedef uint32_t pte_t;
typedef pte_t *pagetable_t;

enum {
    PTE_V = 1u << 0,
    PTE_R = 1u << 1,
    PTE_W = 1u << 2,
    PTE_X = 1u << 3,
    PTE_U = 1u << 4,
    PTE_G = 1u << 5,
    PTE_A = 1u << 6,
    PTE_D = 1u << 7,
};

#define SATP_MODE_SV32 1u
#define SATP_MODE_SHIFT 31u
#define SATP_PPN_MASK 0x003fffffu

static inline uint32_t vm_page_round_down(uint32_t addr)
{
    return addr & ~(VM_PAGE_SIZE - 1u);
}

static inline uint32_t vm_page_round_up(uint32_t addr)
{
    return (addr + VM_PAGE_SIZE - 1u) & ~(VM_PAGE_SIZE - 1u);
}

static inline uint32_t sv32_vpn1(uint32_t va)
{
    return (va >> 22) & 0x3ffu;
}

static inline uint32_t sv32_vpn0(uint32_t va)
{
    return (va >> 12) & 0x3ffu;
}

static inline uint32_t sv32_pte_ppn(pte_t pte)
{
    return (pte >> 10) & SATP_PPN_MASK;
}

static inline pte_t sv32_pte(uintptr_t pa, uint32_t flags)
{
    return (pte_t)(((pa >> VM_PAGE_SHIFT) << 10) | flags);
}

static inline uintptr_t sv32_pte_pa(pte_t pte)
{
    return (uintptr_t)sv32_pte_ppn(pte) << VM_PAGE_SHIFT;
}

static inline uint32_t sv32_satp(uintptr_t root_pa)
{
    return (SATP_MODE_SV32 << SATP_MODE_SHIFT) |
           ((uint32_t)(root_pa >> VM_PAGE_SHIFT) & SATP_PPN_MASK);
}

void vm_init(void);
int vm_map(pagetable_t root, uintptr_t va, uintptr_t pa, size_t size, uint32_t flags);
pagetable_t vm_kernel_table(void);
pagetable_t vm_create_user_table(void);
void vm_switch(pagetable_t root);
void vm_enable_kernel_paging(void);

#endif
