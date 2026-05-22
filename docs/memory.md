# Memory Management

ZOS M3 uses a simple QEMU `virt` memory model so paging can be learned without a
device tree parser.

## Physical Layout

- RAM base: `0x80000000`
- Kernel load base: `0x80200000`
- Conservative RAM top: `0x88000000` (128 MiB)
- UART0 MMIO: `0x10000000`
- Page size: 4096 bytes

The physical page allocator starts at `PAGE_ALIGN_UP(__kernel_end)` and manages
pages up to `0x88000000`.

## Physical Page Allocator

The allocator stores a singly linked free list inside free pages. It exposes:

- `pmm_init()`
- `pmm_alloc_page()`
- `pmm_free_page()`
- `pmm_total_pages()`
- `pmm_free_pages()`
- `pmm_self_test()`

Invalid frees and double frees panic. `NULL` frees are ignored.

## Sv32 Paging

The first page table is an identity map. That keeps the current kernel addresses
valid before and after paging is enabled.

Mapped regions:

- UART0 `0x10000000..0x10001000`
- RAM `0x80000000..0x88000000`

`vm_enable_kernel_paging()` writes Sv32 `satp` and executes `sfence.vma`.

## User Address Spaces

M8 gives each process its own Sv32 root page table. The kernel RAM and UART
identity mappings are copied from the kernel page table, while user text and
stack mappings point at pages owned by the process.

User layout remains fixed so simple raw binaries can keep using the same linker
script:

- user text base: `0x00400000`
- user stack page: `0x00410000..0x00411000`

`fork` copies the parent user pages into child-owned physical pages. `exec`
replaces only the calling process text image. M9 releases exited child text and
stack pages during `wait`/parent wakeup. Full page-table page reclamation is
still deferred to a later VM cleanup pass.
