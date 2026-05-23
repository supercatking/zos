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

## Kernel Heap

M18 adds a small fixed-size kernel allocator for kernel objects that do not need
full pages. It exposes:

- `kmalloc_init()`
- `kmalloc(size)`
- `kfree(ptr)`
- `kmalloc_self_test()`

The first implementation uses size classes of 16, 32, 64, 128, and 256 bytes.
Each class is refilled from whole physical pages, and `kfree()` returns objects
to the matching class free list. It does not yet return fully empty pages to the
physical page allocator, and allocations larger than 256 bytes still return
`NULL`.

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
- user heap: `0x00412000..0x00416000`

`fork` copies the parent user pages into child-owned physical pages. `exec`
replaces only the calling process text image. M9 releases exited child text and
stack pages during `wait`/parent wakeup. Full page-table page reclamation is
still deferred to a later VM cleanup pass.

M18 adds `sbrk(increment)` and a libc-lite `u_malloc/u_free` wrapper. The first
user heap is a fixed four-page region mapped eagerly into every process.
`fork` copies heap pages and the current break, while `exec` clears the heap and
resets the break to `USER_HEAP_BASE`. Heap growth beyond `USER_HEAP_TOP`
returns `-1`; demand paging is still deferred.

User-mode illegal instruction and access/page-fault traps now terminate the
faulting non-init process instead of panicking the kernel. `/bin/badtest` is the
regression case for this path: it writes to an unmapped address, the kernel
marks it exited, and the shell continues.
