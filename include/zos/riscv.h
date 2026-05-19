#ifndef ZOS_RISCV_H
#define ZOS_RISCV_H

#include <zos/types.h>

#define SSTATUS_SIE (1u << 1)
#define SSTATUS_SPIE (1u << 5)
#define SSTATUS_SPP (1u << 8)
#define SSTATUS_SUM (1u << 18)

#define SIE_SSIE (1u << 1)
#define SIE_STIE (1u << 5)
#define SIE_SEIE (1u << 9)

#define SIP_SSIP (1u << 1)
#define SIP_STIP (1u << 5)
#define SIP_SEIP (1u << 9)

#define SCAUSE_INTERRUPT (1u << 31)
#define SCAUSE_CODE_MASK (~SCAUSE_INTERRUPT)
#define SCAUSE_SUPERVISOR_SOFTWARE_INTERRUPT 1u
#define SCAUSE_SUPERVISOR_TIMER_INTERRUPT 5u
#define SCAUSE_SUPERVISOR_EXTERNAL_INTERRUPT 9u
#define SCAUSE_USER_ECALL 8u

#define STVEC_MODE_DIRECT 0u
#define STVEC_MODE_VECTORED 1u

#define csr_read(csr)                                      \
    ({                                                     \
        uintptr_t value;                                   \
        __asm__ volatile("csrr %0, " #csr : "=r"(value)); \
        value;                                             \
    })

#define csr_write(csr, value)                                      \
    do {                                                           \
        uintptr_t __csr_value = (uintptr_t)(value);                \
        __asm__ volatile("csrw " #csr ", %0" : : "rK"(__csr_value)); \
    } while (0)

#define csr_set(csr, bits)                                         \
    do {                                                           \
        uintptr_t __csr_bits = (uintptr_t)(bits);                  \
        __asm__ volatile("csrs " #csr ", %0" : : "rK"(__csr_bits)); \
    } while (0)

#define csr_clear(csr, bits)                                       \
    do {                                                           \
        uintptr_t __csr_bits = (uintptr_t)(bits);                  \
        __asm__ volatile("csrc " #csr ", %0" : : "rK"(__csr_bits)); \
    } while (0)

static inline uintptr_t r_sstatus(void) { return csr_read(sstatus); }
static inline void w_sstatus(uintptr_t value) { csr_write(sstatus, value); }
static inline uintptr_t r_sie(void) { return csr_read(sie); }
static inline void w_sie(uintptr_t value) { csr_write(sie, value); }
static inline uintptr_t r_sip(void) { return csr_read(sip); }
static inline void w_sip(uintptr_t value) { csr_write(sip, value); }
static inline uintptr_t r_stvec(void) { return csr_read(stvec); }
static inline void w_stvec(uintptr_t value) { csr_write(stvec, value); }
static inline uintptr_t r_sepc(void) { return csr_read(sepc); }
static inline void w_sepc(uintptr_t value) { csr_write(sepc, value); }
static inline uintptr_t r_scause(void) { return csr_read(scause); }
static inline uintptr_t r_stval(void) { return csr_read(stval); }
static inline uintptr_t r_sscratch(void) { return csr_read(sscratch); }
static inline void w_sscratch(uintptr_t value) { csr_write(sscratch, value); }
static inline uintptr_t r_time(void) { return csr_read(time); }

static inline void intr_on(void) { csr_set(sstatus, SSTATUS_SIE); }
static inline void intr_off(void) { csr_clear(sstatus, SSTATUS_SIE); }

#endif
