#include <zos/sbi.h>
#include <zos/timer.h>
#include <zos/console.h>

#define VIRT_TIMEBASE_HZ 10000000ull
#define TIMER_INTERVAL (VIRT_TIMEBASE_HZ / TIMER_HZ)

static volatile uint64_t ticks;

static inline uint32_t read_time_low(void)
{
    uint32_t value;

    __asm__ volatile("rdtime %0" : "=r"(value));
    return value;
}

static inline uint32_t read_time_high(void)
{
    uint32_t value;

    __asm__ volatile("rdtimeh %0" : "=r"(value));
    return value;
}

static uint64_t read_time(void)
{
    uint32_t hi;
    uint32_t lo;
    uint32_t hi2;

    do {
        hi = read_time_high();
        lo = read_time_low();
        hi2 = read_time_high();
    } while (hi != hi2);

    return ((uint64_t)hi << 32) | lo;
}

static void timer_schedule_next(void)
{
    (void)sbi_set_timer(read_time() + TIMER_INTERVAL);
}

void timer_init(void)
{
    ticks = 0;
    timer_schedule_next();
}

void timer_handle_interrupt(void)
{
    ticks++;
    if ((ticks % TIMER_HZ) == 0) {
        console_puts("timer: tick ");
        console_put_hex((uintptr_t)ticks);
        console_puts("\n");
    }
    timer_schedule_next();
}

uint64_t timer_ticks(void)
{
    return ticks;
}
