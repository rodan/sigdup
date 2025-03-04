
#include <msp430.h>
#include <inttypes.h>
#include <string.h>
#include "proj.h"
#include "driverlib.h"
#include "fram_glue.h"
#include "atlas430.h"

#if defined (__TI_COMPILER_VERSION__)
#pragma PERSISTENT(HIGH_FRAM_START)
const uint32_t HIGH_FRAM_START =
#elif defined (__IAR_SYSTEMS_ICC__)
__persistent uint32_t HIGH_FRAM_START =
#elif defined (__GNUC__)
__attribute__ ((section (".persistent"))) uint32_t HIGH_FRAM_START =
#endif
        HIGH_FRAM_ADDR;

uint8_t *fram_ptr;
fram_header fram_hdr;

void fram_init(void)
{
    if (SMCLK_FREQ > 8000000) {
        // set WAIT states = 1
        //FRAMCtl_A_configureWaitStateControl(NWAITS_1);
        FRCTL0 = FRCTLPW | NWAITS_1;
        //FRCTL0 = FRCTLPW | AUTO;
        GCCTL1 = 0;
    }

    fram_ptr = (uint8_t *) HIGH_FRAM_START;
    memset(&fram_hdr, 0, sizeof(fram_hdr));
}

int8_t fram_seek(const uint32_t rel_address)
{
    fram_ptr = (uint8_t *) HIGH_FRAM_START + rel_address;
    return EXIT_SUCCESS;
}

/*
int16_t fram_read(uint8_t *buf, const uint16_t count)
{
    buf = fram_ptr;
    fram_ptr += count;
    return EXIT_SUCCESS;
}
*/

int16_t fram_write(uint8_t *buf, const uint16_t count)
{
    uint16_t bytes_remaining = count;
    FRAMCtl_A_write8(buf, fram_ptr, bytes_remaining);
    fram_ptr += count;
    return count;
}

void fram_write_byte(uint8_t byte)
{
    *fram_ptr++ = byte;
}

int8_t fram_write_header(const fram_header *hdr)
{
    const uint16_t len = sizeof(fram_hdr);
    fram_hdr.file_start = HIGH_FRAM_START + len;
    fram_hdr.file_sz = hdr->file_sz;

    if (fram_write((uint8_t *) &fram_hdr, len) == len) {
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}
