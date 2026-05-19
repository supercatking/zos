    .section .rodata.user_init, "a"
    .globl __user_init_start
    .globl __user_init_end

__user_init_start:
    li a0, 1
1:
    auipc a1, %pcrel_hi(user_message)
    addi a1, a1, %pcrel_lo(1b)
    li a2, 24
    li a7, 1
    ecall

    li a0, 0
    li a7, 2
    ecall

2:
    j 2b

user_message:
    .ascii "user: hello from U-mode\n"
user_message_end:

    .balign 4
__user_init_end:
