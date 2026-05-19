#include <zos/sbi.h>

struct sbi_ret sbi_call(uintptr_t eid, uintptr_t fid, uintptr_t arg0,
                        uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                        uintptr_t arg4, uintptr_t arg5)
{
    register uintptr_t a0 __asm__("a0") = arg0;
    register uintptr_t a1 __asm__("a1") = arg1;
    register uintptr_t a2 __asm__("a2") = arg2;
    register uintptr_t a3 __asm__("a3") = arg3;
    register uintptr_t a4 __asm__("a4") = arg4;
    register uintptr_t a5 __asm__("a5") = arg5;
    register uintptr_t a6 __asm__("a6") = fid;
    register uintptr_t a7 __asm__("a7") = eid;

    __asm__ volatile("ecall"
                     : "+r"(a0), "+r"(a1)
                     : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6),
                       "r"(a7)
                     : "memory");

    return (struct sbi_ret){(long)a0, (long)a1};
}

struct sbi_ret sbi_set_timer(uint64_t stime_value)
{
    uintptr_t lo = (uintptr_t)stime_value;
    uintptr_t hi = (uintptr_t)(stime_value >> 32);

    return sbi_call(SBI_EXT_TIME, SBI_FID_SET_TIMER, lo, hi, 0, 0, 0, 0);
}

struct sbi_ret sbi_system_reset(uint32_t reset_type, uint32_t reset_reason)
{
    return sbi_call(SBI_EXT_SRST, SBI_FID_SRST_RESET, reset_type,
                    reset_reason, 0, 0, 0, 0);
}

void sbi_shutdown(void)
{
    (void)sbi_system_reset(SBI_SRST_RESET_TYPE_SHUTDOWN,
                           SBI_SRST_RESET_REASON_NONE);
    (void)sbi_call(SBI_EXT_LEGACY_SHUTDOWN, 0, 0, 0, 0, 0, 0, 0);
}

void sbi_reboot(void)
{
    (void)sbi_system_reset(SBI_SRST_RESET_TYPE_COLD_REBOOT,
                           SBI_SRST_RESET_REASON_NONE);
}

void sbi_console_putchar(char ch)
{
    (void)sbi_call(SBI_EXT_LEGACY_CONSOLE_PUTCHAR, 0, (uintptr_t)ch, 0, 0, 0,
                   0, 0);
}
