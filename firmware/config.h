#ifndef _CONFIG_H_
#define _CONFIG_H_

#define USE_XT1
//#define USE_XT2

#define UART0_SPEED_9600
//#define UART0_SPEED_19200
//#define UART0_SPEED_38400
//#define UART0_SPEED_57600
//#define UART0_SPEED_115200

// enable in order to have the following led setup:
// led 2 - on during timer interrupt operation
// led 3 - on during meter uart operation
// led 4 - on when uC is outside of LPM3 (low power mode)
//#define LED_SYSTEM_STATES

//#define CONFIG_DEBUG

#define CPU_FREQ_8M
//#define CPU_FREQ_16M

#endif
