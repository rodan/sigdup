
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
    TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__8; // divide SMCLK by 8
#if defined (CPU_FREQ_8M)
    TA1EX0 = TAIDEX_3; // further divide SMCLK by 4
#elif defined (CPU_FREQ_16M)
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

void timer_a1_fake_ccr2_event(void)
{
    timer_a1_last_event = TIMER_A1_EVENT_CCR2;
}

void timer_a1_delay_noblk_ccr1(uint16_t ticks)
{
    TA1CCTL1 &= ~CCIE;
    TA1CCTL1 = 0;
    TA1CCR1 = TA1R + ticks;
    TA1CCTL1 = CCIE;
}

void timer_a1_delay_noblk_ccr1_disable(void)
{
    TA1CCTL1 &= ~CCIE;
    TA1CCTL1 = 0;   
}

void timer_a1_delay_ccr2(uint16_t ticks)
{
    TA1CCTL2 &= ~CCIE;
    TA1CCTL2 = 0;
    TA1CCR2 = TA1R + ticks;
    TA1CCTL2 = CCIE;

    timer_a1_last_event &= ~TIMER_A1_EVENT_CCR2;
    while (1) {
        //_BIS_SR(LPM3_bits + GIE);
        //__no_operation();
        if (timer_a1_last_event & TIMER_A1_EVENT_CCR2)
            break;
    }
    TA1CCTL2 &= ~CCIE;
    timer_a1_last_event &= ~TIMER_A1_EVENT_CCR2;
}

void timer_a1_sleep(const uint16_t ms)
{
    uint32_t ticks, d, r;
    uint16_t i;

    ticks = 250 * (uint32_t) ms;

    if (ticks < 65000) {
        timer_a1_delay_ccr2(ticks - TA1_SLEEP_MS_COMPENSATION);
    } else {
        d = ticks / 65000;
        r = ticks % 65000;

        for (i = d; i > 0; i--) {
            timer_a1_delay_ccr2(65000 - TA1_SLEEP_TICKS_COMPENSATION);
        }
        if (r > TA1_SLEEP_TICKS_COMPENSATION) {
            timer_a1_delay_ccr2(r - TA1_SLEEP_TICKS_COMPENSATION);
        }
    }
}

void timer_a1_sleep_ticks(const uint16_t ticks)
{
    timer_a1_delay_ccr2(ticks - TA1_SLEEP_TICKS_COMPENSATION);
}



__attribute__ ((interrupt(TIMER1_A1_VECTOR)))
void timer1_A1_ISR(void)
{
#ifdef LED_SYSTEM_STATES
    sig2_on;
#endif
    uint16_t iv = TA1IV;
    if (iv == TAIV__TACCR1) {
        // timer used by timer_a1_delay_noblk_ccr1()
        // disable interrupt
        TA1CCTL1 &= ~CCIE;
        TA1CCTL1 = 0;
        timer_a1_last_event |= TIMER_A1_EVENT_CCR1;
        _BIC_SR_IRQ(LPM3_bits);
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
