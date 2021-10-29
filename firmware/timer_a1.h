#ifndef __TIMER_A1_H__
#define __TIMER_A1_H__

#include "proj.h"

#define TIMER_A1_EVENT_NONE  0
#define TIMER_A1_EVENT_CCR1  0x1

void timer_a1_init(const uint8_t clk_divider);

uint8_t timer_a1_get_event(void);
void timer_a1_rst_event(void);

uint32_t timer_a1_get_stream_start(void);
uint32_t timer_a1_get_stream_end(void);
void timer_a1_set_stream_pos(const uint32_t address);
void timer_a1_set_stream_start(const uint32_t address);
void timer_a1_set_stream_end(const uint32_t address);

#endif
