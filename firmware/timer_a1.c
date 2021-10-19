
//   timer a0 handling
//   CCR1 is used for timer_a1_delay_noblk_ccr1()
//   CCR2 is used for timer_a1_delay_noblk_ccr2()
//
//   author:          Petre Rodan <2b4eda@subdimension.ro>
//   available from:  https://github.com/rodan/
//   license:         BSD

//#include "config.h"
#include "timer_a1.h"
#include "pg.h"

volatile uint8_t timer_a1_last_event;
volatile uint16_t timer_a1_ovf;

volatile uint32_t stream_pos;   /// FRAM address for the next stream data pkt
volatile uint32_t stream_start; /// FRAM address where the stream starts
volatile uint32_t stream_end;   /// FRAM address where the stream ends

volatile uint8_t next_sig = 0;
volatile uint16_t next_ccr = 0;

uint32_t timer_a1_get_stream_pos(void)
{
    return stream_pos;
}

void timer_a1_set_stream_pos(const uint32_t address)
{
    stream_pos = address;
}

uint32_t timer_a1_get_stream_start(void)
{
    return stream_start;
}

void timer_a1_set_stream_start(const uint32_t address)
{
    stream_start = address;
}

uint32_t timer_a1_get_stream_end(void)
{
    return stream_end;
}

void timer_a1_set_stream_end(const uint32_t address)
{
    stream_end = address;
}

// clk_divider can be one of the CLK_DIV_X defines
void timer_a1_init(const uint8_t clk_divider)
{

    __disable_interrupt();
    timer_a1_ovf = 0;
    stream_pos = 0;
    stream_end = 0;
    switch (clk_divider) {
        case CLK_DIV_1:
            // 0.0625us per clock tick, overflow at 4.096ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__1; // divide by 1
            TA1EX0 = TAIDEX_0; // further divide by 1
            break;
        case CLK_DIV_2:
            // 0.125us per clock tick, overflow at 8.192ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__1; // divide by 1
            TA1EX0 = TAIDEX_1; // further divide by 2
            break;
        case CLK_DIV_3:
            // 0.1875us per clock tick, overflow at 12.29ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__1; // divide by 1
            TA1EX0 = TAIDEX_2; // further divide by 3
            break;
        case CLK_DIV_4:
            // 0.25us per clock tick, overflow at 16.384ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__1; // divide by 1
            TA1EX0 = TAIDEX_3; // further divide by 1
            break;
        case CLK_DIV_5:
            // 0.3125us per clock tick, overflow at 20.48ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__1; // divide by 1
            TA1EX0 = TAIDEX_4; // further divide by 5
            break;
        case CLK_DIV_6:
            // 0.375us per clock tick, overflow at 24.58ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__1; // divide by 1
            TA1EX0 = TAIDEX_5; // further divide by 6
            break;
        case CLK_DIV_7:
            // 0.4375us per clock tick, overflow at 28.67ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__1; // divide by 1
            TA1EX0 = TAIDEX_6; // further divide by 7
            break;
        case CLK_DIV_8:
            // 0.5us per clock tick, overflow at 32.77ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__1; // divide by 1
            TA1EX0 = TAIDEX_7; // further divide by 8
            break;
        case CLK_DIV_10:
            // 0.625us per clock tick, overflow at 40.96ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__2; // divide by 2
            TA1EX0 = TAIDEX_4; // further divide by 5
            break;
        case CLK_DIV_12:
            // 0.75us per clock tick, overflow at 49.15ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__2; // divide by 2
            TA1EX0 = TAIDEX_5; // further divide by 6
            break;
        case CLK_DIV_14:
            // 0.875us per clock tick, overflow at 57.34ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__2; // divide by 2
            TA1EX0 = TAIDEX_6; // further divide by 7
            break;
        case CLK_DIV_16:
            // 1us per clock tick, overflow at 65.54ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__2; // divide by 2
            TA1EX0 = TAIDEX_7; // further divide by 8
            break;
        case CLK_DIV_20:
            // 1.25us per clock tick, overflow at 81.92ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__4; // divide by 4
            TA1EX0 = TAIDEX_4; // further divide by 5
            break;
        case CLK_DIV_24:
            // 1.5us per clock tick, overflow at 98.3ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__4; // divide by 4
            TA1EX0 = TAIDEX_5; // further divide by 6
            break;
        case CLK_DIV_28:
            // 1.75us per clock tick, overflow at 114.7ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__4; // divide by 4
            TA1EX0 = TAIDEX_6; // further divide by 7
            break;
        case CLK_DIV_32:
            // 2us per clock tick, overflow at 131.1ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__8; // divide by 8
            TA1EX0 = TAIDEX_3; // further divide by 4
            break;
        case CLK_DIV_40:
            // 2.5us per clock tick, overflow at 163.8ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__8; // divide by 8
            TA1EX0 = TAIDEX_4; // further divide by 5
            break;
        case CLK_DIV_48:
            // 3us per clock tick, overflow at 196.6ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__8; // divide by 8
            TA1EX0 = TAIDEX_5; // further divide by 6
            break;
        case CLK_DIV_56:
            // 3.5us per clock tick, overflow at 229.4ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__8; // divide by 8
            TA1EX0 = TAIDEX_6; // further divide by 7
            break;
        case CLK_DIV_64:
            // 4us per clock tick, overflow at 262.1ms
            TA1CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__8; // divide by 8
            TA1EX0 = TAIDEX_7; // further divide by 8
            break;
    }
    TA1CTL |= TACLR;
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
    replay_packet_8ch_t *pkt_8ch;
    uint16_t iv = TA1IV;

    if (iv == TAIV__TACCR1) {
        //sig0_on;
        P3OUT = next_sig;
        TA1CCTL1 = 0;
        //next_ccr += 10000;
        TA1CCR1 = next_ccr;
        //TA1CCR1 = 10000;
        TA1CCTL1 = CCIE;

        stream_pos += 3;
        if (stream_pos < stream_end) {
            //next_sig = *((uint8_t *) stream_pos);
            //next_ccr = *((uint16_t *) (stream_pos + 1));
            pkt_8ch = (replay_packet_8ch_t *) stream_pos;
            next_sig = pkt_8ch->sig;
            next_ccr = pkt_8ch->ccr;
        } else {
            TA1CCTL1 = 0;
            stream_pos = stream_start - 3;
        }
        //sig0_off;

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
