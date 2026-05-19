#include <zos/console.h>
#include <zos/panic.h>
#include <zos/pmm.h>
#include <zos/riscv.h>
#include <zos/types.h>
#include <zos/vm.h>

static pte_t *kernel_root;

static void vm_zero_page(void *page)
{
    uint32_t *words = (uint32_t *)page;

    for (size_t i = 0; i < VM_PAGE_SIZE / sizeof(uint32_t); i++) {
        words[i] = 0;
    }
}

static void *vm_alloc_page_table(void)
{
    void *page = pmm_alloc_page();
    if (page != 0) {
        vm_zero_page(page);
    }
    return page;
}

static pte_t *vm_walk_create(pagetable_t root, uintptr_t va)
{
    pte_t *l1 = &root[sv32_vpn1((uint32_t)va)];

    if ((*l1 & PTE_V) == 0) {
        pte_t *l0 = (pte_t *)vm_alloc_page_table();
        if (l0 == 0) {
            return 0;
        }

        *l1 = sv32_pte((uintptr_t)l0, PTE_V);
    }

    if ((*l1 & (PTE_R | PTE_W | PTE_X)) != 0) {
        return 0;
    }

    return &((pte_t *)sv32_pte_pa(*l1))[sv32_vpn0((uint32_t)va)];
}

int vm_map(pagetable_t root, uintptr_t va, uintptr_t pa, size_t size, uint32_t flags)
{
    uintptr_t page_va;
    uintptr_t page_pa;
    uintptr_t end;

    if (root == 0 || size == 0) {
        return -1;
    }

    if ((va & (VM_PAGE_SIZE - 1u)) != 0 || (pa & (VM_PAGE_SIZE - 1u)) != 0) {
        return -1;
    }

    end = va + size;
    if (end < va) {
        return -1;
    }

    page_va = va;
    page_pa = pa;
    while (page_va < end) {
        pte_t *pte = vm_walk_create(root, page_va);
        if (pte == 0 || (*pte & PTE_V) != 0) {
            return -1;
        }

        *pte = sv32_pte(page_pa, flags | PTE_V | PTE_A | PTE_D);
        page_va += VM_PAGE_SIZE;
        page_pa += VM_PAGE_SIZE;
    }

    return 0;
}

pagetable_t vm_kernel_table(void)
{
    return kernel_root;
}

void vm_init(void)
{
    if (kernel_root != 0) {
        return;
    }

    kernel_root = (pte_t *)vm_alloc_page_table();
    if (kernel_root == 0) {
        PANIC("vm: out of page table pages");
    }

    if (vm_map(kernel_root,
               VM_UART0_BASE,
               VM_UART0_BASE,
               VM_PAGE_SIZE,
               PTE_R | PTE_W | PTE_G) != 0) {
        PANIC("vm: map uart0 failed");
    }

    if (vm_map(kernel_root,
               VM_RAM_BASE,
               VM_RAM_BASE,
               VM_RAM_END - VM_RAM_BASE,
               PTE_R | PTE_W | PTE_X | PTE_G) != 0) {
        PANIC("vm: map ram failed");
    }

    console_puts("vm: kernel page table ready\n");
}

void vm_enable_kernel_paging(void)
{
    uint32_t satp;

    if (kernel_root == 0) {
        vm_init();
    }

    satp = sv32_satp((uintptr_t)kernel_root);
    csr_write(satp, satp);
    __asm__ volatile("sfence.vma" ::: "memory");
}
