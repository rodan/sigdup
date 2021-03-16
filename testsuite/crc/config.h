#ifndef _CONFIG_H_
#define _CONFIG_H_

// calculate the crc32 after each received byte instead of at the end of the frame
#define ZMODEM_O_BYTESIZE_CRC32

// save transfered bytes one by one thus minimizing used RAM space
// this might be dead slow on HOST mode, but for fram it's great
#define ZMODEM_O_BYTESIZE_WRITE

#define DEBUG

#endif
