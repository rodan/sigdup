/* crc.c */

#include <stdint.h>
#include "zcrc.h"
#include "config.h"

static uint16_t crc16_table[] =
{
    0x0000,
    0x1021,
    0x2042,
    0x3063,
    0x4084,
    0x50a5,
    0x60c6,
    0x70e7,
    0x8108,
    0x9129,
    0xa14a,
    0xb16b,
    0xc18c,
    0xd1ad,
    0xe1ce,
    0xf1ef
};


#ifdef HW_CRC32
#include <msp430.h>
#else
static uint32_t crc32_table[] =
{
    0x00000000,
    0x1DB71064,
    0x3B6E20C8,
    0x26D930AC,
    0x76DC4190,
    0x6B6B51F4,
    0x4DB26158,
    0x5005713C,
    0xEDB88320,
    0xF00F9344,
    0xD6D6A3E8,
    0xCB61B38C,
    0x9B64C2B0,
    0x86D3D2D4,
    0xA00AE278,
    0xBDBDF21C
};
#endif

uint16_t crc16(const void *data, uint16_t length, uint16_t crc)
{
    const uint8_t *buffer = data;
    while (length--)
    {
        crc = crc16_table[(crc >> 12) ^ (*buffer >> 4)]   ^ (crc << 4);
        crc = crc16_table[(crc >> 12) ^ (*buffer & 0x0f)] ^ (crc << 4);
        buffer++;
    }
    return crc;
}

#ifdef HW_CRC32
uint32_t crc32(const void *data, uint16_t length, uint32_t crc)
{
    const uint8_t *buffer = data;
    uint16_t i;
    crc = ~crc;

    // this function is not able to continue on an already-calculated crc
    CRC32INIRESW1 = (uint32_t) (crc >> 16) & 0x0000FFFF;
    CRC32INIRESW0 = (uint32_t) crc & 0x0000FFFF;

    for (i = 0; i < length; i++) {
        CRC32DIW0_L = (uint16_t) *buffer;
        buffer++;
    }

    crc = ((uint32_t) CRC32RESRW0 << 16);
    crc = ((uint32_t) CRC32RESRW1 & 0x0000FFFF) | crc;

    return ~crc;
}

#else
uint32_t crc32(const void *data, uint16_t length, uint32_t crc)
{
    const uint8_t *buffer = data;
    crc = ~crc;
    while (length--)
    {
        crc = crc32_table[(crc ^  *buffer      ) & 0x0f] ^ (crc >> 4);
        crc = crc32_table[(crc ^ (*buffer >> 4)) & 0x0f] ^ (crc >> 4);
        buffer++;
    }
    return ~crc;
}
#endif


