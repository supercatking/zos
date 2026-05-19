    .section .text.boot, "ax"
    .globl _start
    .type _start, @function

_start:
    la sp, boot_stack_top

    mv s0, a0
    mv s1, a1

    la t0, __bss_start
    la t1, __bss_end
1:
    bgeu t0, t1, 2f
    sw zero, 0(t0)
    addi t0, t0, 4
    j 1b

2:
    mv a0, s0
    mv a1, s1
    call kernel_main

3:
    wfi
    j 3b

    .section .bss.stack, "aw", @nobits
    .balign 16
boot_stack:
    .skip 16384
boot_stack_top:
