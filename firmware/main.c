
#include <msp430.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "proj.h"
#include "driverlib.h"
#include "glue.h"
#include "uart0.h"
#include "ui.h"
#include "timer_a0.h"
#include "timer_a1.h"
#include "timer_a2.h"
#include "uart0.h"
#include "uart3.h"
#include "zmodem.h"
#include "version.h"
#include "fram_glue.h"

volatile uint8_t port5_last_event;

uint8_t tcounter = 0;

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

    sig0_on;

#ifdef USE_XT1
    PJSEL0 |= BIT4 | BIT5;
#ifdef USE_XT2
    PJSEL0 |= BIT6 | BIT7;
    CS_setExternalClockSource(32768, 16000000);
#else
    CS_setExternalClockSource(32768, 0);
#endif
#else
#ifdef USE_XT2
    PJSEL0 |= BIT6 | BIT7;
    CS_setExternalClockSource(0, 16000000);
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
    // Set DCO Frequency
    #if defined (CPU_FREQ_8M)
    CS_setDCOFreq(CS_DCORSEL_0, CS_DCOFSEL_6); // 8MHz
    #elif defined (CPU_FREQ_16M)
    CS_setDCOFreq(CS_DCORSEL_1, CS_DCOFSEL_4); // 16MHz
    #endif
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
#endif

#ifdef USE_XT1
    CS_turnOnLFXT(CS_LFXT_DRIVE_0);
#endif

#ifdef USE_XT2
    CS_turnOnHFXT(CS_HFXT_DRIVE_16MHZ_24MHZ);
#endif
}

/*
static void button_55_irq(uint16_t msg)
{
    if (P5IN & BIT5) {
        //sig2_off;
        timer_a2_set_trigger_slot(SCHEDULE_PB_55, 0, TIMER_A2_EVENT_DISABLE);
        //tcounter++;
        //FRAMCtl_A_write8(&tcounter, (uint8_t *)(uintptr_t)HIGH_FRAM_START, 1);
        //FRAMCtl_A_write8(&tcounter, (uint8_t *)HIGH_FRAM_ADDR, 1);
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
        //sig0_off;
        timer_a2_set_trigger_slot(SCHEDULE_PB_56, 0, TIMER_A2_EVENT_DISABLE);
    } else {
        timer_a2_set_trigger_slot(SCHEDULE_PB_56, systime() + 100, TIMER_A2_EVENT_ENABLE);
    }
}

static void button_56_long_press_irq(uint16_t msg)
{
    uart0_print("PB56 long\r\n");
}
*/

static void scheduler_irq(uint16_t msg)
{
    timer_a2_scheduler_handler();
}

void zmodem_parse_input(void)
{
    //uint8_t c;
    uint8_t i;
    char *input = uart0_get_rx_buf();
    
    for (i=0; i<uart0_get_p(); i++) {
        zrx_byte(input[i]);
    }
}

static void uart0_rx_irq(uint16_t msg)
{
    uint8_t input_type = uart0_get_input_type();
    sig2_on;

    if (input_type == RX_USER) {
        parse_user_input();
    } else {
        zmodem_parse_input();
    }
    uart0_set_eol();
    //uart0_set_input_type(RX_USER);
    sig2_off;
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

    fram_init();

    timer_a0_init();            // uart timeout
//    timer_a1_init();            // interface - ccr1 - meas interval, ccr2 - blocking delay
    timer_a2_init();            // scheduler, systime()
    uart0_port_init();
    uart0_init();

    uart3_init(BAUDRATE_57600);

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

    sig0_off;
    sig1_off;
    sig2_off;
    sig3_off;
    sig4_off;

    sys_messagebus_register(&uart0_rx_irq, SYS_MSG_UART0_RX);
    //sys_messagebus_register(&uart0_rx_irq, SYS_MSG_TIMERA0_CCR1);
    //sys_messagebus_register(&button_55_irq, SYS_MSG_P55_INT);
    //sys_messagebus_register(&button_56_irq, SYS_MSG_P56_INT);

    //sys_messagebus_register(&button_55_long_press_irq, SYS_MSG_P55_TMOUT_INT);
    //sys_messagebus_register(&button_56_long_press_irq, SYS_MSG_P56_TMOUT_INT);

    sys_messagebus_register(&scheduler_irq, SYS_MSG_TIMERA2_CCR1);

    // Reset IRQ flags
    P5IFG &= ~(BIT5 | BIT6);
    // Enable button interrupt
    P5IE |= BIT5 | BIT6;

    // fake button interrupt so we read the battery voltage
    //button_31_irq(0);

    while (1) {
        // sleep
#ifdef LED_SYSTEM_STATES
        //sig4_off;
#endif
        _BIS_SR(LPM3_bits + GIE);
#ifdef LED_SYSTEM_STATES
        //sig4_on;
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
