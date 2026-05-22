#include <zos/console.h>
#include <zos/elf.h>
#include <zos/panic.h>
#include <zos/types.h>

#define EI_CLASS 4u
#define EI_DATA 5u
#define EI_VERSION 6u

#define ELFCLASS32 1u
#define ELFDATA2LSB 1u
#define EV_CURRENT 1u

#define ELF32_EHDR_SIZE 52u
#define ELF32_PHDR_SIZE 32u

static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read32(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static int range_in_image(uintptr_t offset, uintptr_t len, size_t image_size)
{
    return offset <= image_size && len <= image_size - offset;
}

int elf32_parse(const void *image, size_t size, struct elf32_image *out)
{
    const uint8_t *elf = (const uint8_t *)image;
    uintptr_t phoff;
    uint16_t phentsize;
    uint16_t phnum;

    if (elf == 0 || out == 0 || size < ELF32_EHDR_SIZE) {
        return -1;
    }

    if (elf[0] != 0x7fu || elf[1] != 'E' || elf[2] != 'L' || elf[3] != 'F') {
        return -1;
    }

    if (elf[EI_CLASS] != ELFCLASS32 ||
        elf[EI_DATA] != ELFDATA2LSB ||
        elf[EI_VERSION] != EV_CURRENT) {
        return -1;
    }

    if (read16(elf + 16) != ELF32_ET_EXEC ||
        read16(elf + 18) != ELF32_RISCV_MACHINE ||
        read32(elf + 20) != EV_CURRENT ||
        read16(elf + 40) != ELF32_EHDR_SIZE) {
        return -1;
    }

    phoff = read32(elf + 28);
    phentsize = read16(elf + 42);
    phnum = read16(elf + 44);

    if (phentsize != ELF32_PHDR_SIZE || phnum == 0 ||
        !range_in_image(phoff, (uintptr_t)phentsize * phnum, size)) {
        return -1;
    }

    out->entry = read32(elf + 24);
    out->phoff = phoff;
    out->phnum = phnum;
    return 0;
}

int elf32_load_segment(const void *image, size_t size, uint16_t load_index,
                       struct elf32_segment *out)
{
    struct elf32_image parsed;
    const uint8_t *elf = (const uint8_t *)image;
    uint16_t seen = 0;

    if (out == 0 || elf32_parse(image, size, &parsed) != 0) {
        return -1;
    }

    for (uint16_t i = 0; i < parsed.phnum; i++) {
        const uint8_t *ph = elf + parsed.phoff + (uintptr_t)i * ELF32_PHDR_SIZE;
        uint32_t type = read32(ph);
        uintptr_t offset = read32(ph + 4);
        uintptr_t filesz = read32(ph + 16);
        uintptr_t memsz = read32(ph + 20);

        if (type != ELF32_PT_LOAD) {
            continue;
        }

        if (seen == load_index) {
            if (filesz > memsz || !range_in_image(offset, filesz, size)) {
                return -1;
            }

            out->offset = offset;
            out->vaddr = read32(ph + 8);
            out->filesz = filesz;
            out->memsz = memsz;
            out->flags = read32(ph + 24);
            out->align = read32(ph + 28);
            return 0;
        }
        seen++;
    }

    return -1;
}

void elf32_self_test(void)
{
    uint8_t image[ELF32_EHDR_SIZE + ELF32_PHDR_SIZE];
    struct elf32_image parsed;
    struct elf32_segment segment;

    for (size_t i = 0; i < sizeof(image); i++) {
        image[i] = 0;
    }

    image[0] = 0x7f;
    image[1] = 'E';
    image[2] = 'L';
    image[3] = 'F';
    image[EI_CLASS] = ELFCLASS32;
    image[EI_DATA] = ELFDATA2LSB;
    image[EI_VERSION] = EV_CURRENT;

    image[16] = ELF32_ET_EXEC;
    image[18] = ELF32_RISCV_MACHINE & 0xffu;
    image[19] = (ELF32_RISCV_MACHINE >> 8) & 0xffu;
    image[20] = EV_CURRENT;
    image[24] = 0x00;
    image[25] = 0x00;
    image[26] = 0x40;
    image[28] = ELF32_EHDR_SIZE;
    image[40] = ELF32_EHDR_SIZE;
    image[42] = ELF32_PHDR_SIZE;
    image[44] = 1;

    image[ELF32_EHDR_SIZE + 0] = ELF32_PT_LOAD;
    image[ELF32_EHDR_SIZE + 4] = sizeof(image);
    image[ELF32_EHDR_SIZE + 8] = 0x00;
    image[ELF32_EHDR_SIZE + 10] = 0x40;
    image[ELF32_EHDR_SIZE + 16] = 0;
    image[ELF32_EHDR_SIZE + 20] = 16;
    image[ELF32_EHDR_SIZE + 24] = ELF32_PF_R | ELF32_PF_X;
    image[ELF32_EHDR_SIZE + 28] = 0x00;
    image[ELF32_EHDR_SIZE + 29] = 0x10;

    if (elf32_parse(image, sizeof(image), &parsed) != 0 ||
        parsed.entry != 0x00400000u ||
        elf32_load_segment(image, sizeof(image), 0, &segment) != 0 ||
        segment.vaddr != 0x00400000u ||
        segment.memsz != 16u ||
        segment.filesz != 0u ||
        segment.flags != (ELF32_PF_R | ELF32_PF_X)) {
        PANIC("elf: self-test failed");
    }

    console_puts("elf: parser self-test passed\n");
}
