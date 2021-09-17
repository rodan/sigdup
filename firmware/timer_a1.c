
//   timer a0 handling
//   CCR1 is used for timer_a1_delay_noblk_ccr1()
//   CCR2 is used for timer_a1_delay_noblk_ccr2()
//
//   author:          Petre Rodan <2b4eda@subdimension.ro>
//   available from:  https://github.com/rodan/
//   license:         BSD

#include "config.h"
#include "timer_a1.h"
//#include "uart3.h"

volatile uint8_t timer_a1_last_event;
volatile uint16_t timer_a1_ovf;

uint32_t stream_pos;   /// FRAM address for the next stream data pkt
uint32_t stream_start; /// FRAM address where the stream starts
uint32_t stream_end;   /// FRAM address where the stream ends

uint8_t next_sig = 0;
uint16_t next_ccr = 0;

uint32_t timer_a1_get_stream_pos(const uint32_t address)
{
    return stream_pos;
}

void timer_a1_set_stream_pos(const uint32_t address)
{
    stream_pos = address;
}

uint32_t timer_a1_get_stream_end(const uint32_t address)
{
    return stream_end;
}

void timer_a1_set_stream_end(const uint32_t address)
{
    stream_end = address;
}

void timer_a1_set_stream_start(const uint32_t address)
{
    stream_start = address;
}

void timer_a1_init(void)
{

    // ID__4 & TAIDEX_2
    // 1 tick is TA1_DIVIDER / MCLK == 12/8000000 = 1.5 us
    // overflow happens after .0015*65535 = 98.30 ms

    // ID__8 & TAIDEX_3
    // 1 tick is TA1_DIVIDER / MCLK == 32/8000000 = 4 us
    // overflow happens after .004*65535 = 262.14 ms

    __disable_interrupt();
    timer_a1_ovf = 0;
    stream_pos = 0;
    stream_end = 0;
    TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__8; // divide SMCLK by 8
#if defined (SMCLK_FREQ_8M)
    TA1EX0 = TAIDEX_3; // further divide SMCLK by 4
#elif defined (SMCLK_FREQ_16M)
    TA0EX0 = TAIDEX_7; // further divide SMCLK by 8
#endif
    __enable_interrupt();
}

uint8_t timer_a1_get_event(void)
{
    return timer_a1_last_event;
}

void timer_a1_rst_event(void)
{
    timer_a1_last_event = TIMER_A1_EVENT_NONE;
}

__attribute__ ((interrupt(TIMER1_A1_VECTOR)))
void timer1_A1_ISR(void)
{
#ifdef LED_SYSTEM_STATES
    sig2_on;
#endif
    uint16_t iv = TA1IV;

    if (iv == TAIV__TACCR1) {
        sig0_on;
        P2OUT = next_sig;
        TA1CCTL1 = 0;
        TA1CCR1 = next_ccr;
        TA1CCTL1 = CCIE;
        sig0_off;

        stream_pos += 3;
        if (stream_pos < stream_end) {
            next_sig = *((uint8_t *) stream_pos);
            next_ccr = *((uint16_t *) (stream_pos + 1));
        } else {
            TA1CCTL1 = 0;
            stream_pos = stream_start;
        }

        //timer_a1_last_event |= TIMER_A1_EVENT_CCR1;
        //_BIC_SR_IRQ(LPM3_bits);
    } else if (iv == TAIV__TACCR2) {
        // timer used by timer_a1_delay_noblk_ccr2()
        // disable interrupt
        TA1CCTL2 &= ~CCIE;
        TA1CCTL2 = 0;
        timer_a1_last_event |= TIMER_A1_EVENT_CCR2;
        _BIC_SR_IRQ(LPM3_bits);
    } else if (iv == TA1IV_TA1IFG) {
        TA1CTL &= ~TAIFG;
        timer_a1_ovf++;
        //timer_a1_last_event |= TIMER_A1_EVENT_IFG;
        _BIC_SR_IRQ(LPM3_bits);
    }
#ifdef LED_SYSTEM_STATES
    sig2_off;
#endif
}
