
// signal duplicator
//
//  source: https://github.com/rodan/sigdup
//  author: Petre Rodan <2b4eda@subdimension.ro>
// license: GNU GPLv3

#include <msp430.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "pg.h"
#include "proj.h"
#include "driverlib.h"
#include "atlas430.h"
#include "uart0_extras.h"
#include "ui.h"
#include "timer_a0.h"
#include "timer_a1.h"
#include "timer_a2.h"
#include "zmodem.h"
#include "version.h"
#include "fram_glue.h"
#include "sig.h"

volatile uint8_t port5_last_event;
static uint8_t valid_signal = 0;
uart_descriptor bc; // backchannel uart interface

void main_init(void)
{
    P1OUT = 0;
    P1DIR = 0xff;

    P2OUT = 0;
    P2DIR = 0xff;

    P3OUT = 0;
    P3DIR = 0xff;

    P4OUT = 0;
    P4DIR = 0xff;

    // P55 and P56 are buttons
    P5OUT = 0;
    P5DIR = 0x9f;
    // activate pullup
    P5OUT = 0x60;
    P5REN = 0x60;
    // IRQ triggers on the falling edge
    P5IES = 0x60;

    P6OUT = 0;
    P6DIR = 0xff;

    P7OUT = 0;
    P7DIR = 0xff;

    P8OUT = 0;
    P8DIR = 0xff;

    PJOUT = 0;
    PJDIR = 0xffff;

    // output SMCLK on P3.4
    //P3OUT &= ~BIT4;
    //P3DIR |= BIT4;
    //P3SEL1 |= BIT4;

    sig0_on;
}

static void button_55_irq(uint32_t msg)
{
	replay_signal();
/*
    if (P5IN & BIT5) {
        timer_a2_set_trigger_slot(SCHEDULE_PB_55, 0, TIMER_A2_EVENT_DISABLE);
    } else {
        timer_a2_set_trigger_slot(SCHEDULE_PB_55, systime() + 100, TIMER_A2_EVENT_ENABLE);
    }
*/
}

static void button_55_long_press_irq(uint32_t msg)
{
//    replay_signal();
}

static void button_56_irq(uint32_t msg)
{
    if (P5IN & BIT6) {
        //sig0_off;
        timer_a2_set_trigger_slot(SCHEDULE_PB_56, 0, TIMER_A2_EVENT_DISABLE);
    } else {
        timer_a2_set_trigger_slot(SCHEDULE_PB_56, systime() + 100, TIMER_A2_EVENT_ENABLE);
    }
}

static void button_56_long_press_irq(uint32_t msg)
{
    uart_print(&bc, "PB56 long\r\n");
}


static void scheduler_irq(uint32_t msg)
{
    timer_a2_scheduler_handler();
}

static void uart_rx_irq(uint32_t msg)
{
    uint8_t rx;
    uint8_t input_type = uart0_get_input_type();
    struct ringbuf *rbr = uart_get_rx_ringbuf(&bc);

    if (input_type == RX_USER) {
        parse_user_input();
    } else {
        // read the entire ringbuffer and send to zmodem parser
        while (ringbuf_get(rbr, &rx)) {
            zrx_byte(rx);
        }
    }
}

static void zmodem_rcvrdy_irq(uint32_t msg)
{
    prepare_signal();
}

void check_events(void)
{
    uint32_t msg = SYS_MSG_NULL;
    uint16_t ev;

    // uart RX
    if (uart_get_event(&bc) & UART_EV_RX) {
        msg |= SYS_MSG_UART0_RX;
        uart_rst_event(&bc);
    }
    // timer_a0
    ev = timer_a0_get_event();
    if (ev) {
        if (ev & TIMER_A0_EVENT_CCR1) {
            msg |= SYS_MSG_TIMERA0_CCR1;
        } else if (ev & TIMER_A0_EVENT_CCR2) {
            msg |= SYS_MSG_TIMERA0_CCR2;
        }
        timer_a0_rst_event();
    }
    // timer_a2
    ev = timer_a2_get_event();
    if (ev) {
        if (ev & TIMER_A2_EVENT_CCR1) {
            msg |= SYS_MSG_TIMERA2_CCR1;
        }
        timer_a2_rst_event();
    }
    // timer_a2-based scheduler
    ev = timer_a2_get_event_schedule();
    if (ev) {
        if (ev & (1 << SCHEDULE_PB_55)) {
            msg |= SYS_MSG_P55_TMOUT_INT;
        }
        if (ev & (1 << SCHEDULE_PB_56)) {
            msg |= SYS_MSG_P56_TMOUT_INT;
        }
        timer_a2_rst_event_schedule();
    }
    // push button P5.x
    if (port5_last_event) {
        if (port5_last_event & BIT5) {
            msg |= SYS_MSG_P55_INT;
            port5_last_event ^= BIT5;
        }
        if (port5_last_event & BIT6) {
            msg |= SYS_MSG_P56_INT;
            port5_last_event ^= BIT6;
        }
    }
    // zmodem 
    ev = zmodem_get_event();
    if (ev) {
        msg |= SYS_MSG_ZMODEM_RCVRDY;
        zmodem_rst_event();
    }

    eh_exec(msg);
}

void prepare_signal(void)
{
    fram_header *hdr;
    replay_header_t *replay_hdr;

    hdr = (fram_header *)(uintptr_t) HIGH_FRAM_ADDR;
    replay_hdr = (replay_header_t *) hdr->file_start;
    valid_signal = 0;

    // perform checks on FRAM data
    if (replay_hdr->version != VER_REPLAY) {
        uart_print(&bc, "wrong version\r\n");
        sig0_on;
        return;
    }

    if (replay_hdr->header_size + (replay_hdr->packet_count * replay_hdr->bytes_per_packet) != hdr->file_sz) {
        uart_print(&bc, "stream length error\r\n");
        sig0_on;
        return;
    }

    uart_print(&bc, "signal ok\r\n");
    valid_signal = 1;

    timer_a1_init(replay_hdr->clk_divider);
    timer_a1_set_stream_pos(hdr->file_start + replay_hdr->header_size);
    timer_a1_set_stream_start(hdr->file_start + replay_hdr->header_size);
    timer_a1_set_stream_end(hdr->file_start + hdr->file_sz);
}

void replay_signal(void)
{
    TA1CCTL1 &= ~CCIE;
    TA1CCTL1 = 0;
    TA1CTL |= TACLR;
    TA1CCR1 = 1;
    TA1CCTL1 = CCIE;

    // disable scheduling timer, otherwise we get 
    // 6us jitter in the replay signal every 10ms
    TA2CCTL1 = 0;

    // disable button interrupt
    P5IE &= ~BIT5;
}

int main(void)
{
    // stop watchdog
    WDTCTL = WDTPW | WDTHOLD;
    main_init();

    clock_pin_init();
    clock_init();

    fram_init();

    timer_a0_init();            // uart timeout
    timer_a1_init(CLK_DIV_64);  // main signal replication timer
    timer_a2_init();            // scheduler, systime()

    bc.baseAddress = EUSCI_A0_BASE;
    bc.baudrate = BAUDRATE_57600;
    uart_pin_init(&bc);
    uart_init(&bc);
    uart_set_rx_irq_handler(&bc, uart0_extra_irq_handler);

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

    sig0_off;
    sig1_off;
    sig2_off;
    sig3_off;
    sig4_off;

    eh_register(&uart_rx_irq, SYS_MSG_UART0_RX);
    //eh_register(&uart_rx_irq, SYS_MSG_TIMERA0_CCR1);
    eh_register(&button_55_irq, SYS_MSG_P55_INT);
    eh_register(&button_56_irq, SYS_MSG_P56_INT);

    eh_register(&button_55_long_press_irq, SYS_MSG_P55_TMOUT_INT);
    eh_register(&button_56_long_press_irq, SYS_MSG_P56_TMOUT_INT);

    eh_register(&scheduler_irq, SYS_MSG_TIMERA2_CCR1);
    
    eh_register(&zmodem_rcvrdy_irq, SYS_MSG_ZMODEM_RCVRDY);

    // Reset IRQ flags
    P5IFG &= ~(BIT5 | BIT6);
    // Enable button interrupt
    P5IE |= BIT5 | BIT6;

    display_menu();

    while (1) {
        // sleep
#ifdef LED_SYSTEM_STATES
        sig4_off;
#endif
        _BIS_SR(LPM0_bits + GIE);
#ifdef LED_SYSTEM_STATES
        sig4_on;
#endif
        __no_operation();
//#ifdef USE_WATCHDOG
//        WDTCTL = (WDTCTL & 0xff) | WDTPW | WDTCNTCL;
//#endif
        check_events();
        check_events();
        check_events();
    }
}

// Port 5 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT5_VECTOR
__interrupt void port5_isr_handler(void)
#elif defined(__GNUC__)
void __attribute__((interrupt(PORT5_VECTOR))) port5_isr_handler(void)
#else
#error Compiler not supported!
#endif
{
    switch (P5IV) {
    case P5IV__P5IFG5:
        port5_last_event |= BIT5;
        // listen for opposite edge
        P5IES ^= BIT5;
        _BIC_SR_IRQ(LPM3_bits);
        break;
    case P5IV__P5IFG6:
        port5_last_event |= BIT6;
        // listen for opposite edge
        P5IES ^= BIT6;
        _BIC_SR_IRQ(LPM3_bits);
        break;

    }
    P5IFG = 0;
}

#ifdef UNSUPPORTED_COMPILER
    #error "This TI compiler is not supported since I cannot find a way to force it to use a large memory model while also push both data and code to lower memory"
#endif


