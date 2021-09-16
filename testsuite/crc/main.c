
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "zcrc.h"

#define  POLYNOMIAL_32    0xEDB88320

// Holds a crc32 lookup table
uint32_t crc32Table[256];

/* crctab calculated by Mark G. Mendel, Network Systems Corporation */
unsigned short crctab[256] = {
    0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
    0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
    0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
    0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
    0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
    0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
    0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
    0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
    0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
    0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
    0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
    0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
    0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
    0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
    0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
    0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
    0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
    0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
    0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
    0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
    0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
    0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
    0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
    0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
    0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
    0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
    0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
    0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
    0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
    0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
    0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
    0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0
};

const uint32_t CRC32_init = 0xFFFFFFFF;
const uint16_t CRC16_init = 0xFFFF;

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
uint16_t CCITT_Update(uint16_t init, uint16_t input);
uint16_t crc16_msp(const void *data, uint16_t length, uint16_t crc);
uint32_t crc32_msp(const void *data, uint16_t length, uint32_t crc);

// original crc32 function used in lrzsz
uint32_t UPDC32(const uint8_t b, const uint32_t c);
uint16_t UPDC16(const uint8_t cp, const uint16_t crc);
uint16_t crc16_lrzsz(const void *data, uint16_t length, uint16_t crc);
uint32_t crc32_lrzsz(const void *data, uint16_t length, uint32_t crc);

int main(void)
{
    uint32_t lib_crc32_res = 0;
    uint32_t msp_crc32_res = 0;
    uint32_t lrzsz_crc32_res = 0;
    uint32_t crc32_test0 = 0;
    uint32_t crc32_test1 = 0;
    uint32_t crc32_test2 = 0;
    uint32_t crc32_test3 = 0;
    uint16_t lib_crc16_res = 0;
    uint16_t msp_crc16_res = 0;
    uint16_t lrzsz_crc16_res = 0;

    uint16_t i;

    initSwCrc32Table();

    // lib
    lib_crc16_res = zcrc16(in1, sizeof(in1), 0);
    lib_crc32_res = zcrc32(in1, sizeof(in1), 0);


    // msp430
    msp_crc16_res = 0; //CRC16_init;
    for (i = 0; i < sizeof(in1); i++) {
        // Calculate the CRC32 on the low-byte first
        msp_crc16_res = CCITT_Update(msp_crc16_res, (in1[i]));
    }

    msp_crc32_res = CRC32_init;
    for (i = 0; i < sizeof(in1); i++) {
        // Calculate the CRC32 on the low-byte first
        msp_crc32_res = updateSwCrc32(msp_crc32_res, (in1[i]));
    }
    msp_crc32_res = ~msp_crc32_res;


    // lrzsz
    lrzsz_crc16_res = 0; //CRC16_init;
    for (i = 0; i < sizeof(in1); i++) {
        lrzsz_crc16_res = UPDC16((in1[i]), lrzsz_crc16_res);
    }

    lrzsz_crc32_res = CRC32_init;
    for (i = 0; i < sizeof(in1); i++) {
        // Calculate the CRC32 on the low-byte first
        lrzsz_crc32_res = UPDC32((in1[i]), lrzsz_crc32_res);
    }
    lrzsz_crc32_res = ~lrzsz_crc32_res;

#ifdef SHOW_TABLE
    for (i = 0; i < 256; i++) {
        printf("0x%08x, ", crc32Table[i]);
        if ((i & 0x7) == 0x7) {
            printf("\r\n");
        }
    }
#endif

    // test0
    crc32_test0 = crc32_msp(in1, sizeof(in1), 0);
    crc32_test0 = crc32_msp(in2_extra, sizeof(in2_extra), crc32_test0);

    // test1
    crc32_test1 = zcrc32(in1, sizeof(in1), 0);
    crc32_test1 = zcrc32(in2_extra, 1, crc32_test1);

    // test2
    crc32_test2 = zcrc32(in1, sizeof(in1), 0);
    crc32bs_start(crc32_test2);
    crc32bs_upd(in2_extra[0]);
    crc32_test2 = crc32bs_end();

    // test3
    crc32bs_start(0);
    for (i=0;i<sizeof(in2);i++) {
        crc32bs_upd(in2[i]);
    }
    crc32_test3 = crc32bs_end();

    printf("\r\nCRC32 tests\r\n");
    printf("  the following tests should return 0x9be3e0a3:\r\n");
    printf("    crc32(in1) lib result is        0x%x\r\n", lib_crc32_res);
    printf("    crc32(in1) m1 430 result is     0x%x\r\n", msp_crc32_res);
    printf("    crc32(in1) m2 430 result is     0x%x\r\n", crc32_msp(in1, sizeof(in1), 0));
    printf("    crc32(in1) m1 lrzsz result is   0x%x\r\n", lrzsz_crc32_res);
    printf("    crc32(in1) m2 lrzsz result is   0x%x\r\n", crc32_lrzsz(in1, sizeof(in1), 0));
    printf("\r\n  the following tests should return 0xcbf53a1c:\r\n");
    printf("    crc32(in2) m2 lrzsz result is   0x%x\r\n", crc32_lrzsz(in2, sizeof(in2), 0));
    printf("    result of test0 is              0x%x\r\n", crc32_test0);
    printf("    result of test1 is              0x%x\r\n", crc32_test1);
    printf("    result of test2 is              0x%x\r\n", crc32_test2);
    printf("    result of test3 is              0x%x\r\n", crc32_test3);

    printf("\r\nCRC16 tests\r\n");
    printf("  the following tests should return 0xd789:\r\n");
    printf("    crc16(in1) lib result is        0x%x\r\n", lib_crc16_res);
    printf("    crc16(in1) m1 430 result is     0x%x\r\n", msp_crc16_res);
    printf("    crc16(in1) m2 430 result is     0x%x\r\n", crc16_msp(in1, sizeof(in1), 0));
    printf("    crc16(in1) m1 lrzsz result is   0x%x\r\n", lrzsz_crc16_res);
    printf("    crc16(in1) m2 lrzsz result is   0x%x\r\n", crc16_lrzsz(in1, sizeof(in1), 0));


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

uint16_t CCITT_Update(uint16_t init, uint16_t input)
{
    uint16_t CCITT = (uint8_t) (init >> 8) | (init << 8);
    CCITT ^= input;
    CCITT ^= (unsigned char) (CCITT & 0xFF) >> 4;
    CCITT ^= (CCITT << 8) << 4;
    CCITT ^= ((CCITT & 0xFF) << 4) << 1;
    return CCITT;
}

uint16_t crc16_msp(const void *data, uint16_t length, uint16_t crc)
{
    const uint8_t *buffer = data;
    //crc = ~crc;
    while (length--) {
        crc = CCITT_Update(crc, *buffer);
        buffer++;
    }
    //return ~crc;
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

uint16_t UPDC16(const uint8_t cp, const uint16_t crc)
{
    return (crctab[((crc >> 8) & 255)] ^ (crc << 8) ^ cp);
}

uint32_t UPDC32(const uint8_t b, const uint32_t c)
{
    return (crc32Table[((uint32_t) c ^ b) & 0xff] ^ ((c >> 8) & 0x00FFFFFF));
}

uint16_t crc16_lrzsz(const void *data, uint16_t length, uint16_t crc)
{
    const uint8_t *buffer = data;
    //crc = ~crc;
    while (length--) {
        crc = UPDC16(*buffer, crc);
        buffer++;
    }
    //return ~crc;
    return crc;
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
