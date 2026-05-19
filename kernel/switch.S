    .section .text.switch, "ax"
    .globl thread_switch
    .type thread_switch, @function

    .equ CTX_RA, 0
    .equ CTX_SP, 4
    .equ CTX_S0, 8
    .equ CTX_S1, 12
    .equ CTX_S2, 16
    .equ CTX_S3, 20
    .equ CTX_S4, 24
    .equ CTX_S5, 28
    .equ CTX_S6, 32
    .equ CTX_S7, 36
    .equ CTX_S8, 40
    .equ CTX_S9, 44
    .equ CTX_S10, 48
    .equ CTX_S11, 52

thread_switch:
    sw ra, CTX_RA(a0)
    sw sp, CTX_SP(a0)
    sw s0, CTX_S0(a0)
    sw s1, CTX_S1(a0)
    sw s2, CTX_S2(a0)
    sw s3, CTX_S3(a0)
    sw s4, CTX_S4(a0)
    sw s5, CTX_S5(a0)
    sw s6, CTX_S6(a0)
    sw s7, CTX_S7(a0)
    sw s8, CTX_S8(a0)
    sw s9, CTX_S9(a0)
    sw s10, CTX_S10(a0)
    sw s11, CTX_S11(a0)

    lw ra, CTX_RA(a1)
    lw sp, CTX_SP(a1)
    lw s0, CTX_S0(a1)
    lw s1, CTX_S1(a1)
    lw s2, CTX_S2(a1)
    lw s3, CTX_S3(a1)
    lw s4, CTX_S4(a1)
    lw s5, CTX_S5(a1)
    lw s6, CTX_S6(a1)
    lw s7, CTX_S7(a1)
    lw s8, CTX_S8(a1)
    lw s9, CTX_S9(a1)
    lw s10, CTX_S10(a1)
    lw s11, CTX_S11(a1)
    ret

    .size thread_switch, . - thread_switch
