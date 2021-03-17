#ifndef _CONFIG_H_
#define _CONFIG_H_

// calculate the crc32 after each received byte instead of at the end of the frame
#define ZMODEM_O_BYTESIZE_CRC16
#define ZMODEM_O_BYTESIZE_CRC32

// save transfered bytes one by one thus minimizing used RAM space
// this might be dead slow on HOST mode, but for FRAM it's great
#define ZMODEM_O_BYTESIZE_WRITE

// if this option is enabled then the receiver specifies a buffer length which should not be overflown by the sender.
// the communication is resumed only after the receiver ACKs that the data was saved to storage.
// in practice not many terminals have this feature implemented - works great with ZOC, it's completely missing from lrzsz
#define ZMODEM_SEGMENTED_STREAMING

#define FILE_MAX_SZ 0x33FF0
#define FILE_MIN_SZ 0x7f

#define DEBUG

#endif
