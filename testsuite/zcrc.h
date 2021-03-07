/* crc.h */

#ifndef __ZCRC_H__
#define __ZCRC_H__

uint16_t crc16(const void *data, int length, uint16_t crc);
uint32_t crc32(const void *data, int length, uint32_t crc);

#endif