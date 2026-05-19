    .section .rodata.user_init, "a"
    .globl __user_init_start
    .globl __user_init_end

    .macro load_addr reg, sym
1:
    auipc \reg, %pcrel_hi(\sym)
    addi \reg, \reg, %pcrel_lo(1b)
    .endm

__user_init_start:
shell_loop:
    li a0, 1
    load_addr a1, prompt
    li a2, 5
    li a7, 1
    ecall

    li a0, 0
    load_addr a1, input_buf
    li a2, 80
    li a7, 3
    ecall
    mv s0, a0

    load_addr t2, input_buf
    lb t0, 0(t2)
    li t1, '\n'
    beq t0, t1, shell_loop
    li t1, 'h'
    beq t0, t1, cmd_help
    li t1, 'e'
    beq t0, t1, cmd_echo
    li t1, 'l'
    beq t0, t1, cmd_ls
    li t1, 'c'
    beq t0, t1, cmd_cat
    li t1, 'p'
    beq t0, t1, cmd_ps
    li t1, 's'
    beq t0, t1, cmd_sleep
    li t1, 'k'
    beq t0, t1, cmd_kill
    li t1, 'r'
    beq t0, t1, cmd_reboot
    j cmd_unknown

cmd_help:
    li a0, 1
    load_addr a1, help_msg
    li a2, 48
    li a7, 1
    ecall
    j shell_loop

cmd_echo:
    li a0, 1
    load_addr a1, input_buf
    mv a2, s0
    li a7, 1
    ecall
    j shell_loop

cmd_ls:
    li a0, 1
    load_addr a1, ls_msg
    li a2, 21
    li a7, 1
    ecall
    j shell_loop

cmd_cat:
    load_addr a0, readme_path
    li a7, 4
    ecall
    blt a0, zero, cmd_cat_fail
    mv s1, a0

    mv a0, s1
    load_addr a1, input_buf
    li a2, 80
    li a7, 3
    ecall
    mv s2, a0

    mv a0, s1
    li a7, 5
    ecall

    li a0, 1
    load_addr a1, input_buf
    mv a2, s2
    li a7, 1
    ecall
    j shell_loop

cmd_cat_fail:
    li a0, 1
    load_addr a1, cat_fail_msg
    li a2, 16
    li a7, 1
    ecall
    j shell_loop

cmd_ps:
    li a0, 1
    load_addr a1, ps_msg
    li a2, 18
    li a7, 1
    ecall
    j shell_loop

cmd_unknown:
    li a0, 1
    load_addr a1, unknown_msg
    li a2, 16
    li a7, 1
    ecall
    j shell_loop

cmd_sleep:
    li a0, 5
    li a7, 6
    ecall
    li a0, 1
    load_addr a1, sleep_msg
    li a2, 11
    li a7, 1
    ecall
    j shell_loop

cmd_kill:
    li a0, 1
    li a7, 7
    ecall
    li a0, 1
    load_addr a1, kill_msg
    li a2, 18
    li a7, 1
    ecall
    j shell_loop

cmd_reboot:
    li a0, 0
    li a7, 2
    ecall

halt:
    j halt

prompt:
    .ascii "zos$ "
help_msg:
    .ascii "commands: help echo ls cat ps sleep kill reboot\n"
ls_msg:
    .ascii "/README\n/proc/status\n"
readme_path:
    .asciz "/README"
cat_fail_msg:
    .ascii "cat: not found\n"
ps_msg:
    .ascii "pid  name\n1    sh\n"
sleep_msg:
    .ascii "sleep done\n"
kill_msg:
    .ascii "kill: pid 1 noted\n"
unknown_msg:
    .ascii "unknown command\n"

    .balign 4
input_buf:
    .space 80

    .balign 4
__user_init_end:
