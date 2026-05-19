#ifndef ZOS_TIMER_H
#define ZOS_TIMER_H

#include <zos/types.h>

#define TIMER_HZ 100u

void timer_init(void);
void timer_handle_interrupt(void);
uint64_t timer_ticks(void);

#endif
