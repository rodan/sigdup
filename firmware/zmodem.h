/* zmodem.h */

#ifndef ZMODEM_H
#define ZMODEM_H

#define ZPAD 0x2a
#define ZDLE 0x18

#define ZMODEM_BUFFER_SIZE 1024
#define ZMODEM_TIMEOUT 10 /* seconds */

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

#endif /* ZMODEM_H */
