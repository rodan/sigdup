#ifndef _CONFIG_H_
#define _CONFIG_H_

//#define SMCLK_FREQ_8M
#define SMCLK_FREQ_16M

#define USE_XT1
// the devboard has a 16M crystal installed
#ifdef SMCLK_FREQ_16M
#define USE_XT2
#endif

#define USE_UART0
//#define USE_UART3

//#define BAUD_9600
//#define BAUD_19200
//#define BAUD_38400
#define BAUD_57600

#define UART0_RXBUF_SZ 128
#define UART0_RX_USES_RINGBUF

#define UART0_TXBUF_SZ 32
#define UART0_TX_USES_IRQ

#ifdef USE_UART3
#define UART3_RXBUF_SZ 8
//#define UART3_RX_USES_RINGBUF

#define UART3_TXBUF_SZ 64
#define UART3_TX_USES_IRQ
#endif

#define FILE_MIN_SZ 0x0b        // at least header size
#define FILE_MAX_SZ 0x33FF0

// hardware-based crc32 for 1024bytes takes 2.23ms @16MHz
// software-based crc32 for the exact same data takes 14.89ms @16MHz
#define HW_CRC16
#define HW_CRC32

// calculate the crc16/32 after each received byte instead of at the end of the frame
#define ZMODEM_O_BYTESIZE_CRC16
#define ZMODEM_O_BYTESIZE_CRC32

// save transfered bytes one by one thus minimizing used RAM space
#define ZMODEM_O_BYTESIZE_WRITE

//#define ZMODEM_SEGMENTED_STREAMING

// enable in order to have the following led setup:
// led 2 - on during timer interrupt operation
// led 3 - on during meter uart operation
// led 4 - on when uC is outside of LPM3 (low power mode)
//#define LED_SYSTEM_STATES

//#define CONFIG_DEBUG
//#define DEBUG

#endif
