
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "zcrc.h"

#define  POLYNOMIAL_32    0xEDB88320

// Holds a crc32 lookup table
uint32_t crc32Table[256];
const uint32_t CRC_Init = 0xFFFFFFFF;
//const uint32_t CRC_Init = 0x0;

uint8_t crc32TableInit = 0;

const uint8_t in1[] = {
    0x31, 0x32, 0x33, 0x34,
};

const uint8_t in2[] = {
    0x31, 0x32, 0x33, 0x34, 0x35
};

const uint8_t in2_extra[] = {
    0x35
};

// crc32 used in the zcrc library
void initSwCrc32Table(void);

// an equivalent crc32 function to the internal msp430 
uint32_t updateSwCrc32(uint32_t crc, char c);
uint32_t crc32_msp(const void *data, uint16_t length, uint32_t crc);

// original crc32 function used in lrzsz
uint32_t UPDC32(const uint8_t b, const uint32_t c);
uint32_t crc32_lrzsz(const void *data, uint16_t length, uint32_t crc);

int main(void)
{
    uint32_t lib_crc32_res = 0;
    uint32_t msp_crc32_res = 0;
    uint32_t lrzsz_crc32_res = 0;
    uint32_t test_crc32_res = 0;
    uint16_t i;

    initSwCrc32Table();

    // lib crc32
    lib_crc32_res = crc32(in1, sizeof(in1), 0);

    // msp430 crc32
    msp_crc32_res = CRC_Init;
    for (i = 0; i < sizeof(in1); i++) {
        // Calculate the CRC32 on the low-byte first
        msp_crc32_res = updateSwCrc32(msp_crc32_res, (in1[i]));
    }

    // lrzsz crc32
    lrzsz_crc32_res = CRC_Init;
    for (i = 0; i < sizeof(in1); i++) {
        // Calculate the CRC32 on the low-byte first
        lrzsz_crc32_res = UPDC32((in1[i]), lrzsz_crc32_res);
    }

#ifdef SHOW_TABLE
    for (i = 0; i < 256; i++) {
        printf("0x%08x, ", crc32Table[i]);
        if ((i & 0x7) == 0x7) {
            printf("\r\n");
        }
    }
#endif

    printf("crc32(in1) lib result is    0x%x\r\n", lib_crc32_res);
    printf("crc32(in1) 430 result is    0x%x\r\n", crc32_msp(in1, sizeof(in1), 0));
    printf("crc32(in1) lrzsz result is  0x%x\r\n", crc32_lrzsz(in1, sizeof(in1), 0));
    printf("crc32(in2) lrzsz result is  0x%x\r\n", crc32_lrzsz(in2, sizeof(in2), 0));

    test_crc32_res = crc32_lrzsz(in1, sizeof(in1), 0);
    test_crc32_res = crc32_lrzsz(in2_extra, sizeof(in2_extra), test_crc32_res);

    printf("test result is  0x%x\r\n", test_crc32_res);

}

//////////////////////////////////////////////
// msp430-based crc32 implementation

uint32_t updateSwCrc32(uint32_t crc, char c)
{
    uint32_t tmp, long_c;

    long_c = 0x000000ffL & (uint32_t) c;

    if (!crc32TableInit) {
        initSwCrc32Table();
    }

    tmp = crc ^ long_c;
    crc = (crc >> 8) ^ crc32Table[tmp & 0xff];

    return crc;
}

uint32_t crc32_msp(const void *data, uint16_t length, uint32_t crc)
{
    const uint8_t *buffer = data;
    crc = ~crc;
    while (length--) {
        crc = updateSwCrc32(crc, *buffer);
        buffer++;
    }
    return ~crc;
}

//////////////////////////////////////////////
// lrzsz-based crc32 implementation

uint32_t UPDC32(const uint8_t b, const uint32_t c)
{
    return (crc32Table[((uint32_t) c ^ b) & 0xff] ^ ((c >> 8) & 0x00FFFFFF));
}

uint32_t crc32_lrzsz(const void *data, uint16_t length, uint32_t crc)
{
    const uint8_t *buffer = data;
    crc = ~crc;
    while (length--) {
        crc = UPDC32(*buffer, crc);
        buffer++;
    }
    //return ~crc;
    return ~crc;
}

// CRC32 table - used by both msp430 and lrzsz
void initSwCrc32Table(void)
{
    int i, j;
    uint32_t crc;

    for (i = 0; i < 256; i++) {
        crc = (uint32_t) i;

        for (j = 0; j < 8; j++) {
            if (crc & 0x00000001L) {
                crc = (crc >> 1) ^ POLYNOMIAL_32;
            } else {
                crc = crc >> 1;
            }
        }
        crc32Table[i] = crc;
    }

    // Set flag that the CRC32 table is initialized
    crc32TableInit = 1;
}
