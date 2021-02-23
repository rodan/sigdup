#ifndef _CONFIG_H_
#define _CONFIG_H_

#define USE_XT1
//#define USE_XT2

//#define UART0_SPEED_9600_1M
//#define UART0_SPEED_19200_1M
//#define UART0_SPEED_38400_1M
//#define UART0_SPEED_57600_1M
//#define UART0_SPEED_115200_1M
//#define UART0_SPEED_9600_8M
//#define UART0_SPEED_19200_8M
//#define UART0_SPEED_38400_8M
#define UART0_SPEED_57600_8M
//#define UART0_SPEED_115200_8M
//#define UART0_SPEED_230400_8M
//#define UART0_SPEED_460800_8M

// enable in order to have the following led setup:
// led 2 - on during timer interrupt operation
// led 3 - on during meter uart operation
// led 4 - on when uC is outside of LPM3 (low power mode)
//#define LED_SYSTEM_STATES

//#define CONFIG_DEBUG


#endif
