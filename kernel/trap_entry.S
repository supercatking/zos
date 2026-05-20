    .section .text.trap, "ax"
    .globl trap_entry
    .type trap_entry, @function

    .equ TF_RA, 0
    .equ TF_SP, 4
    .equ TF_GP, 8
    .equ TF_TP, 12
    .equ TF_T0, 16
    .equ TF_T1, 20
    .equ TF_T2, 24
    .equ TF_S0, 28
    .equ TF_S1, 32
    .equ TF_A0, 36
    .equ TF_A1, 40
    .equ TF_A2, 44
    .equ TF_A3, 48
    .equ TF_A4, 52
    .equ TF_A5, 56
    .equ TF_A6, 60
    .equ TF_A7, 64
    .equ TF_S2, 68
    .equ TF_S3, 72
    .equ TF_S4, 76
    .equ TF_S5, 80
    .equ TF_S6, 84
    .equ TF_S7, 88
    .equ TF_S8, 92
    .equ TF_S9, 96
    .equ TF_S10, 100
    .equ TF_S11, 104
    .equ TF_T3, 108
    .equ TF_T4, 112
    .equ TF_T5, 116
    .equ TF_T6, 120
    .equ TF_SEPC, 124
    .equ TF_SSTATUS, 128
    .equ TF_SCAUSE, 132
    .equ TF_STVAL, 136
    .equ TF_SIZE, 144

trap_entry:
    csrrw sp, sscratch, sp
    bnez sp, 1f
    csrrw sp, sscratch, sp
1:
    addi sp, sp, -TF_SIZE

    sw ra, TF_RA(sp)
    sw t0, TF_T0(sp)
    csrr t0, sscratch
    bnez t0, 2f
    addi t0, sp, TF_SIZE
2:
    sw t0, TF_SP(sp)
    sw gp, TF_GP(sp)
    sw tp, TF_TP(sp)
    sw t1, TF_T1(sp)
    sw t2, TF_T2(sp)
    sw s0, TF_S0(sp)
    sw s1, TF_S1(sp)
    sw a0, TF_A0(sp)
    sw a1, TF_A1(sp)
    sw a2, TF_A2(sp)
    sw a3, TF_A3(sp)
    sw a4, TF_A4(sp)
    sw a5, TF_A5(sp)
    sw a6, TF_A6(sp)
    sw a7, TF_A7(sp)
    sw s2, TF_S2(sp)
    sw s3, TF_S3(sp)
    sw s4, TF_S4(sp)
    sw s5, TF_S5(sp)
    sw s6, TF_S6(sp)
    sw s7, TF_S7(sp)
    sw s8, TF_S8(sp)
    sw s9, TF_S9(sp)
    sw s10, TF_S10(sp)
    sw s11, TF_S11(sp)
    sw t3, TF_T3(sp)
    sw t4, TF_T4(sp)
    sw t5, TF_T5(sp)
    sw t6, TF_T6(sp)

    csrr t0, sepc
    sw t0, TF_SEPC(sp)
    csrr t0, sstatus
    sw t0, TF_SSTATUS(sp)
    csrr t0, scause
    sw t0, TF_SCAUSE(sp)
    csrr t0, stval
    sw t0, TF_STVAL(sp)

    mv a0, sp
    call trap_handler

    lw t0, TF_SEPC(sp)
    csrw sepc, t0
    lw t0, TF_SSTATUS(sp)
    csrw sstatus, t0
    andi t1, t0, 0x100
    bnez t1, 3f
    addi t2, sp, TF_SIZE
    csrw sscratch, t2
    j 4f
3:
    csrw sscratch, zero
4:

    lw ra, TF_RA(sp)
    lw gp, TF_GP(sp)
    lw tp, TF_TP(sp)
    lw t1, TF_T1(sp)
    lw t2, TF_T2(sp)
    lw s0, TF_S0(sp)
    lw s1, TF_S1(sp)
    lw a0, TF_A0(sp)
    lw a1, TF_A1(sp)
    lw a2, TF_A2(sp)
    lw a3, TF_A3(sp)
    lw a4, TF_A4(sp)
    lw a5, TF_A5(sp)
    lw a6, TF_A6(sp)
    lw a7, TF_A7(sp)
    lw s2, TF_S2(sp)
    lw s3, TF_S3(sp)
    lw s4, TF_S4(sp)
    lw s5, TF_S5(sp)
    lw s6, TF_S6(sp)
    lw s7, TF_S7(sp)
    lw s8, TF_S8(sp)
    lw s9, TF_S9(sp)
    lw s10, TF_S10(sp)
    lw s11, TF_S11(sp)
    lw t3, TF_T3(sp)
    lw t4, TF_T4(sp)
    lw t5, TF_T5(sp)
    lw t6, TF_T6(sp)
    lw t0, TF_T0(sp)
    lw sp, TF_SP(sp)
    sret

    .size trap_entry, . - trap_entry
