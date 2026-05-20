OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
    . = 0x00400000;

    .text : ALIGN(4) {
        *(.text.start)
        *(.text .text.*)
    }

    .rodata : ALIGN(4) {
        *(.rodata .rodata.*)
    }

    .data : ALIGN(4) {
        *(.sdata .sdata.*)
        *(.data .data.*)
    }

    .bss : ALIGN(16) {
        *(.sbss .sbss.*)
        *(.bss .bss.*)
        *(COMMON)
    }

    /DISCARD/ : {
        *(.comment)
        *(.riscv.attributes)
        *(.eh_frame)
        *(.note .note.*)
    }
}
