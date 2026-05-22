#ifndef ZOS_ELF_H
#define ZOS_ELF_H

#include <zos/types.h>

#define ELF32_RISCV_MACHINE 243u
#define ELF32_ET_EXEC 2u
#define ELF32_PT_LOAD 1u

#define ELF32_PF_X 1u
#define ELF32_PF_W 2u
#define ELF32_PF_R 4u

struct elf32_image {
    uintptr_t entry;
    uintptr_t phoff;
    uint16_t phnum;
};

struct elf32_segment {
    uintptr_t offset;
    uintptr_t vaddr;
    uintptr_t filesz;
    uintptr_t memsz;
    uintptr_t flags;
    uintptr_t align;
};

int elf32_parse(const void *image, size_t size, struct elf32_image *out);
int elf32_load_segment(const void *image, size_t size, uint16_t load_index,
                       struct elf32_segment *out);
void elf32_self_test(void);

#endif
