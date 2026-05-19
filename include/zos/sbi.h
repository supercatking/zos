#ifndef ZOS_SBI_H
#define ZOS_SBI_H

#include <zos/types.h>

#define SBI_SUCCESS 0

#define SBI_EXT_TIME 0x54494d45u
#define SBI_EXT_SRST 0x53525354u
#define SBI_EXT_LEGACY_CONSOLE_PUTCHAR 0x01u
#define SBI_EXT_LEGACY_SHUTDOWN 0x08u

#define SBI_FID_SET_TIMER 0u
#define SBI_FID_SRST_RESET 0u

#define SBI_SRST_RESET_TYPE_SHUTDOWN 0u
#define SBI_SRST_RESET_TYPE_COLD_REBOOT 1u
#define SBI_SRST_RESET_TYPE_WARM_REBOOT 2u

#define SBI_SRST_RESET_REASON_NONE 0u

struct sbi_ret {
    long error;
    long value;
};

struct sbi_ret sbi_call(uintptr_t eid, uintptr_t fid, uintptr_t arg0,
                        uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                        uintptr_t arg4, uintptr_t arg5);

struct sbi_ret sbi_set_timer(uint64_t stime_value);
struct sbi_ret sbi_system_reset(uint32_t reset_type, uint32_t reset_reason);
void sbi_shutdown(void);
void sbi_reboot(void);
void sbi_console_putchar(char ch);

#endif
