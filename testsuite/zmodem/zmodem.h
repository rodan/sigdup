/* zmodem.h */

#ifndef ZMODEM_H
#define ZMODEM_H

// based on your project's set of constraints you might want to tweak the following defines:
//
// scenario 1. if you want to have tiny RAM usage footprint while having very fast storage capability
//   enable ZMODEM_O_BYTESIZE_WRITE, ZMODEM_O_BYTESIZE_CRC16, ZMODEM_O_BYTESIZE_CRC32
//   disable ZMODEM_SEGMENTED_STREAMING
//   ZMODEM_BUFFER_SIZE can be as low as 128 bytes and go to a maximum of 1024 bytes
//      ( this buffer will hold the non-data headers received and the longest one is the ZFILE header
//        who's size is pretty much governed by the length of the filename )
//   your RX ring buffer can be as low as 16 bytes
//      ( this buffer holds all the incoming data until zmodem is able to process them )
//
//   WARNING: for this scenario to work with the minimal buffers shown your hardware needs to be able to
//            process (handle byte rx interrupt, checksum, write to storage) 1 byte of data faster then 
//            the time needed for 1 byte to be transfered via your chosen bitrate.
//            a logic analyzer should be used to verify this requirement
//
//
// scenario 2. if you have enough RAM to spare but slower processing hardware
//   disable ZMODEM_O_BYTESIZE_WRITE, ZMODEM_O_BYTESIZE_CRC16, ZMODEM_O_BYTESIZE_CRC32
//   enable ZMODEM_SEGMENTED_STREAMING
//   both ZMODEM_BUFFER_SIZE and ZMODEM_FRAME_SIZE should be set to 1024
//       ( in this scenario this buffer will hold headers as before and the entire ZDATA frame )
//   the RX ring buffer should be as large as possible in order to avoid buffer overruns
//       ( this buffer grows specially when the hardware is busy processing a ZDATA frame )
//   
//   WARNING: zmodem's segmented streaming feature will increase the time needed to transfer files 
//            and it's implemented only in a few serial terminal software (ZOC works great with it)
//

// if this option is enabled then the receiver specifies a buffer length which should not be overflowed by the sender.
// the communication is resumed only after the receiver ACKs that the data was saved to storage 
//   - see Forsberg's zmodem.txt protocol document for details.
// in practice not many terminals have this feature implemented - works great with ZOC, it's completely missing from lrzsz
//#define ZMODEM_SEGMENTED_STREAMING

// save transfered bytes one by one to the non-volatile storage in an atempt to minimize used RAM space
//#define ZMODEM_O_BYTESIZE_WRITE

// calculate checksums after each received byte instead of at the end of the frame
//#define ZMODEM_O_BYTESIZE_CRC16
//#define ZMODEM_O_BYTESIZE_CRC32

//#define FILE_MIN_SZ 0x7f
//#define FILE_MAX_SZ 0x33FF0

#define ZPAD 0x2a
#define ZDLE 0x18

#ifdef ZMODEM_O_BYTESIZE_WRITE
// this version of the buffer only needs to hold zmodem headers
#define   ZMODEM_BUFFER_SIZE  128
#define    ZMODEM_FRAME_SIZE  1024
#else
// this version of the buffer holds zmodem headers and the 1kbyte data block
#define   ZMODEM_BUFFER_SIZE  1024
#define    ZMODEM_FRAME_SIZE  ZMODEM_BUFFER_SIZE
#endif

#define    ZMODEM_EVENT_NONE  0
#define  ZMODEM_EVENT_RCVRDY  0x1 /// file fully received 


#ifdef USE_LFS
void *zmodem_init(lfs_t *lfs, lfs_file_t *lfs_file);
#else
void *zmodem_init(void);
#endif

void zmodem_reinit(void);
void zrx_byte(uint8_t byte);
void zmodem_timeout(void);
uint8_t zmodem_progress(void);
uint8_t zmodem_debug(void);
void zmodem_setactive(void);
uint8_t zmodem_active(void);
uint8_t zmodem_waiting(void);

uint8_t zmodem_get_event(void);
void zmodem_rst_event(void);

#endif /* ZMODEM_H */
