
//   timer a0 handling
//   CCR1 is used for timer_a1_delay_noblk_ccr1()
//   CCR2 is used for timer_a1_delay_noblk_ccr2()
//
//   author:          Petre Rodan <2b4eda@subdimension.ro>
//   available from:  https://github.com/rodan/
//   license:         BSD

//#include "config.h"
#include "timer_a1.h"
//#include "uart3.h"

volatile uint8_t timer_a1_last_event;
volatile uint16_t timer_a1_ovf;

volatile uint32_t stream_pos;   /// FRAM address for the next stream data pkt
volatile uint32_t stream_start; /// FRAM address where the stream starts
volatile uint32_t stream_end;   /// FRAM address where the stream ends

volatile uint8_t next_sig = 0;
volatile uint16_t next_ccr = 0;

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

    __disable_interrupt();
    timer_a1_ovf = 0;
    stream_pos = 0;
    stream_end = 0;
    TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__8; // divide SMCLK by 8
#if defined (SMCLK_FREQ_8M)
    TA1EX0 = TAIDEX_0; // further divide SMCLK by 1
#elif defined (SMCLK_FREQ_16M)
    TA1EX0 = TAIDEX_1; // further divide SMCLK by 2
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
        P3OUT = next_sig;
        TA1CCTL1 = 0;
        //next_ccr += 10000;
        TA1CCR1 = next_ccr;
        //TA1CCR1 = 10000;
        TA1CCTL1 = CCIE;

        stream_pos += 3;
        if (stream_pos < stream_end) {
            next_sig = *((uint8_t *) stream_pos);
            next_ccr = *((uint16_t *) (stream_pos + 1));
        } else {
            TA1CCTL1 = 0;
            stream_pos = stream_start;
        }
        sig0_off;

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
