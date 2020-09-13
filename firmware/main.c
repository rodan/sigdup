
/*
    reference MSP430 library for an HI6000 EDGE probe

    tweak the baud rate in config.h
*/

#include <msp430.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "proj.h"
#include "driverlib.h"
#include "glue.h"
//#include "ui.h"
#include "timer_a0.h"
#include "timer_a1.h"
#include "timer_a2.h"
#include "uart0.h"
#include "version.h"

volatile uint8_t port3_last_event;
volatile uint8_t port5_last_event;
uint32_t button_down_start;

void main_init(void)
{
    P1OUT = 0;
    P1DIR = 0xff;

    P2OUT = 0;
    P2DIR = 0xff;

    P3OUT = 0;
    P3DIR = 0xf5;

    // IRQ triggers on rising edge
    P3IES &= ~BIT1;

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

    P8DIR = 0xff;
    P8OUT = 0;

    PJOUT = 0;
    PJDIR = 0xffff;

    // port init
    //P1DIR |= BIT0 + BIT2 + BIT3 + BIT4 + BIT5;

    sig0_on;
    bus_3v3_on;

    // P3.0 is a jumper, make it an input
    //P3DIR &= ~BIT0;
    //P3OUT |= BIT0;
    //P3REN |= BIT0;

#ifdef USE_XT1
    PJSEL0 |= BIT4 | BIT5;
#ifdef USE_XT2
    PJSEL0 |= BIT6 | BIT7;
    CS_setExternalClockSource(32768, 8000000);
#else
    CS_setExternalClockSource(32768, 0);
#endif
#else
#ifdef USE_XT2
    PJSEL0 |= BIT6 | BIT7;
    CS_setExternalClockSource(0, 8000000);
#endif
#endif

#ifdef USE_XT1
    // configure MCLK, SMCLK to be source by DCOCLK
    CS_initClockSignal(CS_ACLK, CS_LFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
#endif

#ifdef USE_XT2
    CS_initClockSignal(CS_SMCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_MCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
#else
    // Set DCO Frequency to 8MHz
    CS_setDCOFreq(CS_DCORSEL_0, CS_DCOFSEL_6);
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
#endif

#ifdef USE_XT1
    CS_turnOnLFXT(CS_LFXT_DRIVE_0);
#endif

#ifdef USE_XT2
    CS_turnOnHFXT(CS_HFXT_DRIVE_8MHZ_16MHZ);
#endif
}

static void button_31_irq(uint16_t msg)
{
    if (P3IN & BIT1) {
        timer_a2_set_trigger_slot(SCHEDULE_PB_31_OFF, systime() + 300, TIMER_A2_EVENT_ENABLE);
        timer_a2_set_trigger_slot(SCHEDULE_POWER_SAVING, systime() + POWER_SAVING_DELAY,
                                  TIMER_A2_EVENT_ENABLE);
        // IRQ triggers on falling edge
        P3IES |= BIT1;
    } else {
        timer_a2_set_trigger_slot(SCHEDULE_PB_31_OFF, 0, TIMER_A2_EVENT_DISABLE);
        // IRQ triggers on rising edge
        P3IES &= ~BIT1;
    }
}

static void button_33_irq(uint16_t msg)
{
    if (P3IN & BIT3) {
        //sig2_on;
    } else {
        //sig2_off;
    }
}

static void button_55_irq(uint16_t msg)
{
    if (P5IN & BIT5) {
        //sig2_off;
        timer_a2_set_trigger_slot(SCHEDULE_PB_55, 0, TIMER_A2_EVENT_DISABLE);
    } else {
        //sig2_on;
        timer_a2_set_trigger_slot(SCHEDULE_PB_55, systime() + 100, TIMER_A2_EVENT_ENABLE);
    }
}

static void button_55_long_press_irq(uint16_t msg)
{
    uart0_print("PB55 long\r\n");
    //sig2_off;
}

static void button_56_irq(uint16_t msg)
{
    if (P5IN & BIT6) {
        sig0_off;
        timer_a2_set_trigger_slot(SCHEDULE_PB_56, 0, TIMER_A2_EVENT_DISABLE);
    } else {
        timer_a2_set_trigger_slot(SCHEDULE_PB_56, systime() + 100, TIMER_A2_EVENT_ENABLE);
    }
}

static void button_56_long_press_irq(uint16_t msg)
{
    uart0_print("PB56 long\r\n");
}

static void poweroff_irq(uint16_t msg)
{
    halt();
}

static void scheduler_irq(uint16_t msg)
{
    timer_a2_scheduler_handler();
}

void halt(void)
{
    bus_5v_off;
    bus_3v3_off;
    sig0_on;
    sig1_off;
    sig2_off;
    sig3_off;
    sig4_off;
}

static void uart0_rx_irq(uint16_t msg)
{
    //parse_user_input();
    uart0_set_eol();
    timer_a2_set_trigger_slot(SCHEDULE_POWER_SAVING, systime() + POWER_SAVING_DELAY,
                              TIMER_A2_EVENT_ENABLE);
}

void check_events(void)
{
    struct sys_messagebus *p = sys_messagebus_getp();
    uint32_t msg = SYS_MSG_NULL;
    uint16_t ev;

    // uart RX
    if (uart0_get_event() & UART0_EV_RX) {
        msg |= SYS_MSG_UART0_RX;
        uart0_rst_event();
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
    // timer_a1
    ev = timer_a1_get_event();
    if (ev) {
        if (ev & TIMER_A1_EVENT_CCR1) {
            msg |= SYS_MSG_TIMERA1_CCR1;
        } else if (ev & TIMER_A1_EVENT_CCR2) {
            msg |= SYS_MSG_TIMERA1_CCR2;
        }
        timer_a1_rst_event();
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
        if ((ev & (1 << SCHEDULE_POWER_SAVING)) || (ev & (1 << SCHEDULE_PB_31_OFF))) {
            msg |= SYS_MSG_SCH_POWEROFF;
        }
        if (ev & (1 << SCHEDULE_LED_ON)) {
            msg |= SYS_MSG_SCH_LED_ON;
        }
        if (ev & (1 << SCHEDULE_LED_OFF)) {
            msg |= SYS_MSG_SCH_LED_OFF;
        }
        if (ev & (1 << SCHEDULE_PB_55)) {
            msg |= SYS_MSG_P55_TMOUT_INT;
        }
        if (ev & (1 << SCHEDULE_PB_56)) {
            msg |= SYS_MSG_P56_TMOUT_INT;
        }
        timer_a2_rst_event_schedule();
    }

    // push button P3.1, P3.3 int
    if (port3_last_event) {
        if (port3_last_event & BIT1) {
            msg |= SYS_MSG_P31_INT;
            port3_last_event ^= BIT1;
        }
        if (port3_last_event & BIT3) {
            msg |= SYS_MSG_P33_INT;
            port3_last_event ^= BIT3;
        }
        port3_last_event = 0;
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

    while (p) {
        // notify listener if he registered for any of these messages
        if (msg & p->listens) {
            p->fn(msg);
        }
        p = p->next;
    }
}

int main(void)
{
    // stop watchdog
    WDTCTL = WDTPW | WDTHOLD;
    main_init();

//    timer_a0_init();            // edge uart timeout
//    timer_a1_init();            // interface - ccr1 - meas interval, ccr2 - blocking delay
//    timer_a2_init();            // systime()
    uart0_port_init();
    uart0_init();

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

    sig0_off;
    sig1_off;
    sig2_off;
    sig3_off;

    sys_messagebus_register(&uart0_rx_irq, SYS_MSG_UART0_RX);
    sys_messagebus_register(&button_31_irq, SYS_MSG_P31_INT);
    sys_messagebus_register(&button_33_irq, SYS_MSG_P33_INT);
    sys_messagebus_register(&button_55_irq, SYS_MSG_P55_INT);
    sys_messagebus_register(&button_56_irq, SYS_MSG_P56_INT);

    sys_messagebus_register(&poweroff_irq, SYS_MSG_SCH_POWEROFF);
    sys_messagebus_register(&button_55_long_press_irq, SYS_MSG_P55_TMOUT_INT);
    sys_messagebus_register(&button_56_long_press_irq, SYS_MSG_P56_TMOUT_INT);

    sys_messagebus_register(&scheduler_irq, SYS_MSG_TIMERA2_CCR1);
    timer_a2_set_trigger_slot(SCHEDULE_POWER_SAVING, POWER_SAVING_DELAY, TIMER_A2_EVENT_ENABLE);

    // Reset IRQ flags
    P3IFG = 0;
    // Enable button interrupt
    P3IE |= BIT1;

    // Reset IRQ flags
    P5IFG &= ~(BIT5 | BIT6);
    // Enable button interrupt
    P5IE |= BIT5 | BIT6;

    // fake button interrupt so we read the battery voltage
    //button_31_irq(0);

    while (1) {
        // sleep
#ifdef LED_SYSTEM_STATES
        sig4_off;
#endif
        _BIS_SR(LPM3_bits + GIE);
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
        check_events();
        check_events();
    }
}

// Port 3 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT3_VECTOR
__interrupt void port3_isr_handler(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT3_VECTOR))) port3_isr_handler(void)
#else
#error Compiler not supported!
#endif
{
    switch (P3IV) {
    case P3IV__P3IFG1:
        port3_last_event |= BIT1;
        // listen for opposite edge
        P3IES ^= BIT1;
        //P3IFG &= ~TRIG1;
        _BIC_SR_IRQ(LPM3_bits);
        break;
    case P3IV__P3IFG3:
        port3_last_event |= BIT3;
        // listen for opposite edge
        P3IES ^= BIT3;
        //P3IFG &= ~TRIG3;
        _BIC_SR_IRQ(LPM3_bits);
        break;
    }
    P3IFG = 0;
}

// Port 5 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT5_VECTOR
__interrupt void port5_isr_handler(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT5_VECTOR))) port5_isr_handler(void)
#else
#error Compiler not supported!
#endif
{
    switch (P5IV) {
    case P5IV__P5IFG5:
        port5_last_event |= BIT5;
        // listen for opposite edge
        P5IES ^= BIT5;
        //P3IFG &= ~TRIG1;
        _BIC_SR_IRQ(LPM3_bits);
        break;
    case P5IV__P5IFG6:
        port5_last_event |= BIT6;
        // listen for opposite edge
        P5IES ^= BIT6;
        //P3IFG &= ~TRIG1;
        _BIC_SR_IRQ(LPM3_bits);
        break;

    }
    P5IFG = 0;
}
