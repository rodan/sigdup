#ifndef __PROJ_H__
#define __PROJ_H__

#include <msp430.h>
#include <stdlib.h>
#include <inttypes.h>
#include "config.h"

#define sig0_on              P1OUT |= BIT0
#define sig0_off             P1OUT &= ~BIT0
#define sig0_switch          P1OUT ^= BIT0

#define sig1_on              P1OUT |= BIT1
#define sig1_off             P1OUT &= ~BIT1
#define sig1_switch          P1OUT ^= BIT1

#define sig2_on              P1OUT |= BIT3
#define sig2_off             P1OUT &= ~BIT3
#define sig2_switch          P1OUT ^= BIT3

#define sig3_on              P1OUT |= BIT4
#define sig3_off             P1OUT &= ~BIT4
#define sig3_switch          P1OUT ^= BIT4

#define sig4_on              P1OUT |= BIT5
#define sig4_off             P1OUT &= ~BIT5
#define sig4_switch          P1OUT ^= BIT5

#define true                1
#define false               0

/*!
	\brief List of possible message types for the message bus.
	\sa sys_messagebus_register()
*/

#define                SYS_MSG_NULL  0
#define        SYS_MSG_TIMERA0_CCR1  0x1      // timer_a0_delay_noblk_ccr1
#define        SYS_MSG_TIMERA0_CCR2  0x2      // timer_a0_delay_noblk_ccr2
#define        SYS_MSG_TIMERA1_CCR1  0x4      // timer_a1_delay_noblk_ccr1
#define        SYS_MSG_TIMERA1_CCR2  0x8      // timer_a1_delay_noblk_ccr2
#define          SYS_MSG_TIMER0_IFG  0x10     // timer0 overflow
#define            SYS_MSG_UART0_RX  0x20     // UART received something
//#define                              0x40     // 
//#define                              0x80     // 
//#define                              0x100    // 
//#define                              0x200    // 
//#define                              0x400    // 
//#define                              0x800    // 
//#define                              0x1000   // 
//#define                              0x2000   // 
//#define                              0x4000   // 
#define             SYS_MSG_P55_INT  0x8000   // P5.5 interrupt
#define             SYS_MSG_P56_INT  0x10000  // P5.6 interrupt
#define       SYS_MSG_P55_TMOUT_INT  0x20000  // P5.5 long press
#define       SYS_MSG_P56_TMOUT_INT  0x40000  // P5.6 long press
#define        SYS_MSG_TIMERA2_CCR1  0x80000  // timer_a2_delay_noblk_ccr1


//#define                             TIMER_A2_SLOT_0
//#define                             TIMER_A2_SLOT_1
//#define                             TIMER_A2_SLOT_2
//#define                             TIMER_A2_SLOT_3
//#define                             TIMER_A2_SLOT_4
//#define                             TIMER_A2_SLOT_5
//#define                             TIMER_A2_SLOT_6
#define              SCHEDULE_PB_55 TIMER_A2_SLOT_7
#define              SCHEDULE_PB_56 TIMER_A2_SLOT_8

void check_events(void);

#endif
