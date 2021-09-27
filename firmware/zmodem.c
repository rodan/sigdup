/* zmodem.c */

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#if HOST
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "config.h"

static int fdout;

#else
#include "proj.h"
#include "uart0.h"
#include "uart3.h"
#include "uart0_extras.h"
#include "fram_glue.h"
#include "lib_convert.h"

//char z_buf[CONV_BASE_10_BUF_SZ];

#endif

#ifdef USE_LFS
#include "lfs.h"
#endif

#include "zmodem.h"
#include "zcrc.h"

#define XON  0x11
#define XOFF 0x13

#define ZCRCE 0x68
#define ZCRCG 0x69
#define ZCRCQ 0x6a
#define ZCRCW 0x6b
#define ZRUB0 0x6c
#define ZRUB1 0x6d

#define ZBIN    0x41
#define ZHEX    0x42
#define ZBIN32  0x43
#define ZBINR32 0x44

#define ZRQINIT     0x00
#define ZRINIT      0x01
#define ZSINIT      0x02
#define ZACK        0x03
#define ZFILE       0x04
#define ZSKIP       0x05
#define ZNAK        0x06
#define ZABORT      0x07
#define ZFIN        0x08
#define ZRPOS       0x09
#define ZDATA       0x0a
#define ZEOF        0x0b
#define ZFERR       0x0c
#define ZCRC        0x0d
#define ZCHALLENGE  0x0e
#define ZCOMPL      0x0f
#define ZCAN        0x10
#define ZFREECNT    0x11
#define ZCOMMAND    0x12
#define ZSTDERR     0x13

#define ZF0_CANFDX  0x01
#define ZF0_CANOVIO 0x02
#define ZF0_CANBRK  0x04
#define ZF0_CANCRY  0x08
#define ZF0_CANLZW  0x10
#define ZF0_CANFC32 0x20
#define ZF0_ESCCTL  0x40
#define ZF0_ESC8    0x80

#define ZF1_CANVHDR 0x01

#define ZF0_TESCCTL 0x40
#define ZF0_TESC8   0x80

#define BUFFER_SIZE ZMODEM_BUFFER_SIZE

struct {
    uint8_t active;
    uint8_t zdlecount;
    uint16_t count;
    uint8_t state;
    uint8_t protostate;
    uint16_t datalen;
    uint8_t frametype;
    uint8_t escape_ctrl;
    uint8_t escape_bit8;
    uint8_t ok;
    uint32_t fileoffset;
    uint32_t transferred;
    uint32_t total;
    uint16_t timeouts;
#ifdef USE_LFS
    lfs_t *lfs;
    lfs_file_t *lfs_file;
#endif
    uint8_t fileopen;
    uint8_t debug;
    void (*process_data)(void);
    uint8_t header[5];
    uint8_t crc[4];
    uint8_t buffer[BUFFER_SIZE+1];
} zstate;

#define STATE_IDLE                0
#define STATE_HEADER              1
#define STATE_HEADER_ZHEX         2
#define STATE_HEADER_ZBIN         3
#define STATE_HEADER_ZBIN32       4
#define STATE_HEADER_ZHEX_CRC     5
#define STATE_HEADER_ZBIN_CRC     6
#define STATE_HEADER_ZBIN32_CRC   7
#define STATE_HEADER_ZHEX_CRLF    8
#define STATE_DATA_ZBIN           9
#define STATE_DATA_ZBIN32        10
#define STATE_DATA_ZBIN_ZCRCE    11
#define STATE_DATA_ZBIN_ZCRCG    12
#define STATE_DATA_ZBIN_ZCRCQ    13
#define STATE_DATA_ZBIN_ZCRCW    14
#define STATE_DATA_ZBIN32_ZCRCE  15
#define STATE_DATA_ZBIN32_ZCRCG  16
#define STATE_DATA_ZBIN32_ZCRCQ  17
#define STATE_DATA_ZBIN32_ZCRCW  18
#define STATE_FINISH             19

#define PROTOSTATE_RX_IDLE           0
#define PROTOSTATE_RX_WAIT           1
#define PROTOSTATE_RX_TRANSFER       2

#ifdef HOST
#ifdef DEBUG
#define ZDEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define ZDEBUG(...)
#endif
void ztx_byte(uint8_t byte);
#define zmodem_exit()
#else
#define ZDEBUG(...)
#define ztx_byte(x) uart0_tx(x)
#define zmodem_exit() uart0_set_input_type(RX_USER)
#endif

#define isxdigit(x) ((((x) >= '0') && ((x) <= '9')) || (((x) >= 'a') && ((x) <= 'f')))
#define hexdecode(x) (((x) < 'a') ? ((x) - '0') : ((x) + 10 - 'a'))
#define hexencode(x) (((x) < 10) ? ((x) + '0') : ((x) - 10 + 'a'))

uint8_t zmodem_open_file(char *filename);
uint8_t zmodem_close_file(void);

void ztx_escbyte(uint8_t byte)
{
    uint8_t escape = false;

    if (zstate.escape_ctrl && (byte < 32))
        escape = true;
    if (zstate.escape_bit8 && (byte >= 0x80))
        escape = true;
    if ((byte & 0x7f) == XON)
        escape = true;
    if ((byte & 0x7f) == XOFF)
        escape = true;
    if (byte == ZDLE)
        escape = true;

    if (escape) {
        ztx_byte(ZDLE);
        byte ^= 0x40;
    }
    ztx_byte(byte);
}

void ztx_hexbyte(uint8_t byte)
{
    uint8_t b1 = byte >> 4;
    uint8_t b2 = byte & 0x0f;
    ztx_byte(hexencode(b1));
    ztx_byte(hexencode(b2));
}

uint32_t _atoi(char* str)
{
    uint8_t i;
    uint32_t ret = 0;

    // uint32_t's length is 10 digits max so don't parse past that
    for (i = 0; (str[i] != '\0') && (str[i] != 0x20) && (i < 10); ++i) {
        if ((str[i] > 0x29) && (str[i] < 0x40)) {
            ret = ret * 10 + str[i] - '0';
        } else {
            return ret;
        }
    }
 
    return ret;
}

void zmodem_enter_state(const uint8_t state)
{
    zstate.count = 0;
    zstate.state = state;
    ZDEBUG("In state %d\n", state);
}

void zmodem_enter_protostate(const uint8_t state)
{
    zstate.protostate = state;
    ZDEBUG("In protocol state %d\n", state);
}

void zmodem_reinit(void)
{
    zstate.active = false;
    zstate.zdlecount = 0;
    zstate.count = 0;
    zstate.state = STATE_IDLE;
    zstate.protostate = PROTOSTATE_RX_IDLE;
    zstate.escape_ctrl = false;
    zstate.escape_bit8 = false;
    zstate.process_data = NULL;
    zstate.timeouts = 0;
    zstate.transferred = 0;
    zstate.total = 0;
    if (zstate.fileopen)
        zmodem_close_file();
}

void *zmodem_init(
#ifdef USE_LFS
                     lfs_t * lfs, lfs_file_t * lfs_file
#else
                     void
#endif
    )
{
#ifdef USE_LFS
    zstate.lfs = lfs;
    zstate.lfs_file = lfs_file;
#endif
    zstate.fileopen = false;
    zstate.debug = 0;
    zmodem_reinit();
    return zstate.buffer;
}

uint8_t zmodem_progress(void)
{
    if (zstate.total == 0)
        return -1;
    if (zstate.transferred > zstate.total)
        return -1;

    return zstate.transferred * 100 / zstate.total;
}

uint8_t zmodem_active(void)
{
    return zstate.active;
}

uint8_t zmodem_waiting(void)
{
    return zstate.timeouts != 0;
}

void zmodem_setactive(void)
{
    zstate.active = true;
}

uint8_t zmodem_debug(void)
{
    return zstate.debug;
}

void zmodem_send_hex_header(const uint8_t type, void *data);

void zmodem_send_zrinit(void)
{
#ifdef ZMODEM_SEGMENTED_STREAMING
    uint8_t header[] = {
        ZMODEM_FRAME_SIZE & 0xff,
        ZMODEM_FRAME_SIZE >> 8,
        0,
        ZF0_CANFC32
    };
#else
    uint8_t header[] = {
        0,
        0,
        0,
        ZF0_CANFC32 | ZF0_CANOVIO
    };
#endif
    zstate.active = true;
    zmodem_send_hex_header(ZRINIT, header);
    zmodem_enter_state(STATE_IDLE);
    zmodem_enter_protostate(PROTOSTATE_RX_WAIT);
}

void zmodem_send_abort(void)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
        ztx_byte(ZDLE);
    for (i = 0; i < 10; i++)
        ztx_byte(8);
}

void zmodem_process_zrqinit(void)
{
    if (zstate.protostate != PROTOSTATE_RX_IDLE)
        return;

    zmodem_send_zrinit();
}

void zmodem_process_zrinit(void)
{
    /* XXX we're getting echoes back, something's gone wrong */
    zmodem_enter_state(STATE_IDLE);
    zmodem_enter_protostate(PROTOSTATE_RX_IDLE);
}

void zmodem_process_zsinit_data(void)
{
    uint8_t header[] = {
        0,
        0,
        0,
        0
    };

    if (zstate.protostate != PROTOSTATE_RX_WAIT)
        return;

    ZDEBUG("zsinit_data: %d\n", zstate.state);
    /* We don't care about the Attn sequence for now */
    switch (zstate.frametype) {
    case ZCRCQ:
    case ZCRCW:
        zmodem_send_hex_header(ZACK, header);
        break;
    default:
        break;
    }
}

void zmodem_enter_data_state(void)
{
    switch (zstate.state) {
    case STATE_HEADER_ZHEX_CRLF:
        zmodem_enter_state(STATE_DATA_ZBIN);
        break;
    case STATE_HEADER_ZBIN_CRC:
#ifdef ZMODEM_O_BYTESIZE_CRC16
        crc16bs_start(0);
#endif
        zmodem_enter_state(STATE_DATA_ZBIN);
        break;
    case STATE_HEADER_ZBIN32_CRC:
#ifdef ZMODEM_O_BYTESIZE_CRC32
        crc32bs_start(0);
#endif
        zmodem_enter_state(STATE_DATA_ZBIN32);
        break;
    }
}

void zmodem_process_zsinit(void)
{
    ZDEBUG("zsinit\n");
    if (zstate.protostate != PROTOSTATE_RX_WAIT)
        return;

    ZDEBUG("zsinit working\n");

    zstate.escape_ctrl = ((zstate.header[4] & ZF0_TESCCTL) != 0);
    zstate.escape_bit8 = ((zstate.header[4] & ZF0_TESC8) != 0);

    zstate.process_data = zmodem_process_zsinit_data;

    zmodem_enter_data_state();
}

void zmodem_process_zack(void)
{
    ;
}

void zmodem_send_rpos(void)
{
    uint8_t header[] = {
        zstate.fileoffset & 0xff,
        (zstate.fileoffset >> 8) & 0xff,
        (zstate.fileoffset >> 16) & 0xff,
        (zstate.fileoffset >> 24) & 0xff
    };
    zmodem_send_hex_header(ZRPOS, header);
}

void zmodem_process_zfile_data(void)
{
    if (zstate.datalen > BUFFER_SIZE)
        return;

    if (zstate.buffer[zstate.datalen - 1] != '\0')
        return;

    if (!zstate.ok) {
        /* XXX what can we do here? Send ZRINIT again? */
        return;
    }

    char *filenamep = (char *)zstate.buffer;
    int filenamelen = strlen(filenamep);
    char *filesize = filenamep + filenamelen + 1;

    /* Check that we have at least one more null terminator */
    if (filenamelen + 1 >= zstate.datalen)
        return;

    zstate.total = zstate.transferred + _atoi(filesize);

    /* Optional - send a ZRCRC to determine where to start from */

    ZDEBUG("Got filename: %s, sz %u\n", zstate.buffer, zstate.total);
    zstate.fileoffset = 0;

    if ((zstate.total < FILE_MIN_SZ) || (zstate.total > FILE_MAX_SZ)) {
        ZDEBUG("! skipping, wrong size \n");
        uint8_t header[] = { 0, 0, 0, 0 };
        /* ZSKIP or ZFERR? */
        zmodem_send_hex_header(ZSKIP, header);
        return;
    }

    if (zmodem_open_file((char *)zstate.buffer)) {
        zmodem_enter_protostate(PROTOSTATE_RX_TRANSFER);
        zmodem_send_rpos();
    } else {
        ZDEBUG("! skipping \n");
        uint8_t header[] = { 0, 0, 0, 0 };
        /* ZSKIP or ZFERR? */
        zmodem_send_hex_header(ZSKIP, header);
    }
}

void zmodem_process_zfile(void)
{
    if (zstate.protostate != PROTOSTATE_RX_WAIT)
        return;

    /* Ignore the conversion options
     * We're not going to do any conversion
     * But maybe later we can take note of ZCRECOV
     * Also ignore the management options
     * We don't have timestamps
     * And we can save some complexity here
     */
    zstate.process_data = zmodem_process_zfile_data;

    zmodem_enter_data_state();
}

void zmodem_process_zskip(void)
{
    /* We shouldn't be getting this as a receiver */
    ;
}

void zmodem_process_znak(void)
{
    /* Last header was garbled */
    zmodem_send_zrinit();
}

void zmodem_process_zabort(void)
{
    /* In theory, we shouldn't get this as a receiver */
    ;
}

void zmodem_process_zfin(void)
{
    if (zstate.protostate != PROTOSTATE_RX_WAIT)
        return;

    uint8_t header[] = {
        0,
        0,
        0,
        0
    };
    zmodem_send_hex_header(ZFIN, header);
    zmodem_enter_state(STATE_FINISH);
}

void zmodem_process_zrpos(void)
{
    /* We shouldn't get this as a receiver */
}

uint8_t zmodem_file_write(void)
{
    int16_t err = -1;

    if (!zstate.fileopen)
        return false;

    /* Write zstate.buffer */
    ZDEBUG("Writing %d bytes to file\n", zstate.datalen);
#if defined(USE_LFS)
    err = lfs_file_write(zstate.lfs, zstate.lfs_file, zstate.buffer, zstate.datalen);
#elif defined(HOST)
#ifndef ZMODEM_O_BYTESIZE_WRITE
    err = write(fdout, zstate.buffer, zstate.datalen);
#else
    err = 0;
#endif
#else

#ifndef ZMODEM_O_BYTESIZE_WRITE
    // FRAM
    err = fram_write(zstate.buffer, zstate.datalen);
    if (err == zstate.datalen) {
        err = 0;
    }
#else
    err = 0;
#endif
#endif

    if (err < 0)
        return false;
    return true;
}

uint8_t zmodem_open_file(char *filename)
{
    int16_t err = -1;
#if defined(USE_LFS)
    err = lfs_file_open(zstate.lfs, zstate.lfs_file, filename, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
#elif defined(HOST)
    fdout = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fdout > 0) {
        err = 0;
    }
#else
    // FRAM
    fram_init();
    fram_header hdr;
    hdr.file_sz = zstate.total;
    err = fram_write_header(&hdr);
#endif

    if (!err)
        zstate.fileopen = true;
    return !err;
}

uint8_t zmodem_close_file(void)
{
    int16_t err = -1;

#if defined(USE_LFS)
    err = lfs_file_close(zstate.lfs, zstate.lfs_file);
#elif defined(HOST)
    err = close(fdout);
#else
    // FRAM
    err = 0;
#endif

    if (!err)
        zstate.fileopen = false;
    return !err;
}

void zmodem_process_zdata_data(void)
{
    if (zstate.ok && zmodem_file_write()) {
        zstate.fileoffset += zstate.datalen;
        /* XXX we may need to revisit this if we ever support resuming */
        zstate.transferred += zstate.datalen;
        ZDEBUG("zstate.state %d\r\nzstate.frametype %x\r\n", zstate.state, zstate.frametype);
        // after a ZCRCQ/ZCRCW frame is received zstate.state gets reset to 0, so the 
        // old switch() based on state never gets to send out the ZACK headers
        switch (zstate.frametype) {
        case ZCRCQ:
        case ZCRCW:
            if (zstate.fileoffset != zstate.total) {
                uint8_t header[] = {
                    zstate.fileoffset & 0xff,
                    (zstate.fileoffset >> 8) & 0xff,
                    (zstate.fileoffset >> 16) & 0xff,
                    (zstate.fileoffset >> 24) & 0xff
                }; 
                zmodem_send_hex_header(ZACK, header);
            }
            break;
        case ZCRCG:
        case ZCRCE:
            // no response is needed unless an error is detected
            break;
        default:
            break;
        }
    } else {
        zmodem_send_rpos();
        zmodem_enter_state(STATE_IDLE);
    }
}

void zmodem_process_zdata(void)
{
    uint32_t offset = ((uint32_t) zstate.header[1]) +
        ((uint32_t) zstate.header[2] << 8) + ((uint32_t) zstate.header[3] << 16) + ((uint32_t) zstate.header[4] << 24);

    if (zstate.protostate != PROTOSTATE_RX_TRANSFER)
        return;

    if (offset == zstate.fileoffset) {
        zmodem_enter_data_state();
        zstate.process_data = zmodem_process_zdata_data;
    } else {
        zmodem_send_rpos();
        /* XXX do we maybe have to receive the ZDATA data first? */
        zmodem_enter_state(STATE_IDLE);
    }
}

void zmodem_process_zeof(void)
{
    uint32_t offset = ((uint32_t) zstate.header[1]) +
        ((uint32_t) zstate.header[2] << 8) + ((uint32_t) zstate.header[3] << 16) + ((uint32_t) zstate.header[4] << 24);

    if (zstate.protostate != PROTOSTATE_RX_TRANSFER)
        return;

    ZDEBUG("EOF: %u, %u\n", offset, zstate.fileoffset);

    if (offset != zstate.fileoffset) {
        zmodem_enter_state(STATE_IDLE);
        return;
    }

    zmodem_close_file();
    zmodem_send_zrinit();
}

void zmodem_process_zferr(void)
{
    /* We shouldn't get this as a receiver */
    ;
}

void zmodem_process_zcrc(void)
{
    /* We're not sending ZRCRC yet, so we shouldn't get it as a response yet */
    ;
}

void zmodem_process_zchallenge(void)
{
    /* We shouldn't get this as a receiver */
}

void zmodem_process_zcompl(void)
{
    /* We shouldn't get this as a receiver */
}

void zmodem_process_zcan(void)
{
    ZDEBUG("Abort sequence received\n");
    zstate.zdlecount = 0;
    zmodem_send_abort();
    zmodem_enter_state(STATE_IDLE);
}

void zmodem_process_zfreecnt(void)
{
    /* unimplemented */
    ;
}

void zmodem_process_zcommand(void)
{
    /* unimplemented */
    ;
}

void zmodem_process_zstderr(void)
{
    /* unimplemented */
    ;
}

void zmodem_process_header(void)
{
    ZDEBUG("Header received\n");
    ZDEBUG("Type: %x\n", zstate.header[0]);
    ZDEBUG("Contents: %x %x %x %x\n", zstate.header[1], zstate.header[2], zstate.header[3], zstate.header[4]);
    zstate.timeouts = 0;
    switch (zstate.header[0]) {
    case ZRQINIT:
        zmodem_process_zrqinit();
        break;
    case ZRINIT:
        zmodem_process_zrinit();
        break;
    case ZSINIT:
        zmodem_process_zsinit();
        break;
    case ZACK:
        zmodem_process_zack();
        break;
    case ZFILE:
        zmodem_process_zfile();
        break;
    case ZSKIP:
        zmodem_process_zskip();
        break;
    case ZNAK:
        zmodem_process_znak();
        break;
    case ZABORT:
        zmodem_process_zabort();
        break;
    case ZFIN:
        zmodem_process_zfin();
#ifdef HOST
        usleep(40);
        exit(0);
#endif
        // exit zmodem parsing mode
        zmodem_exit();
        break;
    case ZRPOS:
        zmodem_process_zrpos();
        break;
    case ZDATA:
        zmodem_process_zdata();
        break;
    case ZEOF:
        zmodem_process_zeof();
        break;
    case ZFERR:
        zmodem_process_zferr();
        break;
    case ZCRC:
        zmodem_process_zcrc();
        break;
    case ZCHALLENGE:
        zmodem_process_zchallenge();
        break;
    case ZCOMPL:
        zmodem_process_zcompl();
        break;
    case ZCAN:
        zmodem_process_zcan();
        break;
    case ZFREECNT:
        zmodem_process_zfreecnt();
        break;
    case ZCOMMAND:
        zmodem_process_zcommand();
        break;
    case ZSTDERR:
        zmodem_process_zstderr();
        break;
    }
}

void zmodem_timeout(void)
{
    if (zstate.protostate == PROTOSTATE_RX_IDLE)
        return;

    if (zstate.state == STATE_FINISH) {
        zmodem_reinit();
        return;
    }

    zstate.timeouts++;

    if (zstate.timeouts > 4) {
        zmodem_reinit();
        return;
    }

    if (zstate.protostate == PROTOSTATE_RX_TRANSFER && zstate.timeouts == 1) {
        zmodem_send_rpos();
        return;
    }

    /* If we're in the middle of a data transfer, we can reset the counter
     * here. We'll end up back at the start anyway, and the counter can count
     * up from there until we abort the whole session.
     */
    if (zstate.protostate == PROTOSTATE_RX_TRANSFER)
        zstate.timeouts = 0;

    zmodem_send_zrinit();
}

void zmodem_send_hex_header(const uint8_t type, void *data)
{
    uint8_t i;
    uint8_t buffer[5];
    uint16_t crc;
    buffer[0] = type & 0x7f;
    for (i = 0; i < 4; i++)
        buffer[i + 1] = ((uint8_t *) data)[i];
    ZDEBUG("Send hex header: %x %x %x %x %x\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
    crc = zcrc16(buffer, 5, 0);
    ztx_byte(ZPAD);
    ztx_byte(ZPAD);
    ztx_byte(ZDLE);
    ztx_byte(ZHEX);
    for (i = 0; i < 5; i++)
        ztx_hexbyte(buffer[i]);
    ztx_hexbyte(crc >> 8);
    ztx_hexbyte(crc & 0xff);
    ztx_byte('\r');
    ztx_byte('\n');
    if (type == ZACK || type == ZFIN)
        return;
    ztx_byte(XON);
}

void zmodem_send_bin_header(const uint8_t type, void *data)
{
    uint8_t i;
    uint8_t buffer[5];
    uint16_t crc;
    buffer[0] = type;
    for (i = 0; i < 4; i++)
        buffer[i + 1] = ((uint8_t *) data)[i];
    ZDEBUG("Send bin header: %x %x %x %x %x\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
    crc = zcrc16(buffer, 5, 0);
    ztx_byte(ZPAD);
    ztx_byte(ZDLE);
    ztx_byte(ZHEX);
    for (i = 0; i < 5; i++)
        ztx_escbyte(buffer[i]);
    ztx_escbyte(crc >> 8);
    ztx_escbyte(crc & 0xff);
}

void zmodem_send_bin32_header(const uint8_t type, void *data)
{
    uint8_t i;
    uint8_t buffer[5];
    uint32_t crc;
    buffer[0] = type;
    for (i = 0; i < 4; i++)
        buffer[i + 1] = ((uint8_t *) data)[i];
    ZDEBUG("Send bin32 header: %x %x %x %x %x\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
    crc = zcrc32(buffer, 5, 0);
    ztx_byte(ZPAD);
    ztx_byte(ZDLE);
    ztx_byte(ZHEX);
    for (i = 0; i < 5; i++)
        ztx_escbyte(buffer[i]);
    ztx_escbyte((crc >> 24) & 0xff);
    ztx_escbyte((crc >> 16) & 0xff);
    ztx_escbyte((crc >> 8) & 0xff);
    ztx_escbyte(crc & 0xff);
}

void zmodem_send_znak(void)
{
    uint8_t zero[4] = { 0, 0, 0, 0 };
    zmodem_send_hex_header(ZNAK, zero);
}

void zmodem_process_data(uint8_t ok)
{
    //uint16_t i;
    ZDEBUG("Data received\n");
    ZDEBUG("CRC %s\n", ok ? "ok" : "not ok");
/*    
    for (i = 0; i < zstate.datalen; i++) {
        ZDEBUG("%x ", zstate.buffer[i]);
        if ((i % 16) == 15) {
            ZDEBUG("\n");
        }
    }
    if (zstate.count % 16)
        ZDEBUG("\n");
*/
    zstate.ok = ok;

    switch (zstate.state) {
    case STATE_DATA_ZBIN_ZCRCE:
    case STATE_DATA_ZBIN32_ZCRCE:
        zmodem_enter_state(STATE_IDLE);
        break;
    case STATE_DATA_ZBIN_ZCRCG:
#ifdef ZMODEM_O_BYTESIZE_CRC16
        crc16bs_start(0);
#endif
        zmodem_enter_state(STATE_DATA_ZBIN);
        break;
    case STATE_DATA_ZBIN32_ZCRCG:
#ifdef ZMODEM_O_BYTESIZE_CRC32
        crc32bs_start(0);
#endif
        zmodem_enter_state(STATE_DATA_ZBIN32);
        break;
    case STATE_DATA_ZBIN_ZCRCQ:
    case STATE_DATA_ZBIN32_ZCRCQ:
        zmodem_enter_state(STATE_IDLE);
        break;
    case STATE_DATA_ZBIN_ZCRCW:
    case STATE_DATA_ZBIN32_ZCRCW:
        zmodem_enter_state(STATE_IDLE);
        break;
    default:
        zmodem_enter_state(STATE_IDLE);
        break;
    }
    if (zstate.process_data != NULL)
        zstate.process_data();
}

void zrx_byte(uint8_t byte)
{
#if defined(ZMODEM_O_BYTESIZE_CRC32) || defined(ZMODEM_O_BYTESIZE_CRC16)
    uint8_t keep_byte = 1;
#endif

    ZDEBUG(" %02x", byte);

    if ((byte & 0x7f) == XON || (byte & 0x7f) == XOFF)
        return;

    if (byte == ZDLE) {
        zstate.zdlecount++;
        if (zstate.zdlecount >= 5) {
            zstate.header[0] = ZCAN;
            zmodem_process_header();
        }
        if (zstate.count && zstate.state == STATE_IDLE) {
            zstate.zdlecount = 0;
            zmodem_enter_state(STATE_HEADER);
        }
    } else {
        if (zstate.zdlecount) {
            switch (byte) {
            case ZRUB0:
                byte = 0x7f;
                break;
            case ZRUB1:
                byte = 0xff;
                break;
            default:
                if ((byte & 0x60) == 0x40)
                    byte ^= 0x40;
                break;
            }
        }
        switch (zstate.state) {
        case STATE_IDLE:
            if (byte == ZPAD)
                zstate.count++;
            else
                zstate.count = 0;
            break;
        case STATE_HEADER:
            switch (byte) {
            case ZHEX:
                zmodem_enter_state(STATE_HEADER_ZHEX);
                break;
            case ZBIN:
                zmodem_enter_state(STATE_HEADER_ZBIN);
                break;
            case ZBIN32:
                zmodem_enter_state(STATE_HEADER_ZBIN32);
                break;
            default:
                zmodem_send_znak();
                zmodem_enter_state(STATE_IDLE);
                break;
            }
            break;
        case STATE_HEADER_ZHEX:
            if (!isxdigit(byte)) {
                zmodem_send_znak();
                zmodem_enter_state(STATE_IDLE);
            }
            byte = hexdecode(byte);
            if (zstate.count & 1)
                zstate.header[zstate.count / 2] += byte;
            else
                zstate.header[zstate.count / 2] = byte << 4;
            zstate.count++;
            if (zstate.count >= 10)
                zmodem_enter_state(STATE_HEADER_ZHEX_CRC);
            break;
        case STATE_HEADER_ZHEX_CRC:
            if (!isxdigit(byte)) {
                zmodem_send_znak();
                zmodem_enter_state(STATE_IDLE);
            }
            byte = hexdecode(byte);
            if (zstate.count & 1)
                zstate.crc[zstate.count / 2] += byte;
            else
                zstate.crc[zstate.count / 2] = byte << 4;
            zstate.count++;
            if (zstate.count >= 4)
                zmodem_enter_state(STATE_HEADER_ZHEX_CRLF);
            break;
        case STATE_HEADER_ZHEX_CRLF:
            if (zstate.count == 0 && (byte & 0x7f) == '\r') {
                zstate.count++;
                break;
            }
            if (zstate.count == 1 && (byte & 0x7f) == '\n') {
                uint16_t crc = zcrc16(zstate.header, 5, 0);
                uint16_t rxcrc = zstate.crc[1] | (zstate.crc[0] << 8);
                ZDEBUG("CRC calculated: %x, received %x\n", crc, rxcrc);
                if (crc == rxcrc) {
                    zmodem_process_header();
                } else {
                    zmodem_send_znak();
                    zmodem_enter_state(STATE_IDLE);
                }
                break;
            }
            zmodem_send_znak();
            zmodem_enter_state(STATE_IDLE);
            break;
        case STATE_HEADER_ZBIN:
        case STATE_HEADER_ZBIN32:
            zstate.header[zstate.count++] = byte;
            if (zstate.count >= 5)
                zmodem_enter_state((zstate.state ==
                                    STATE_HEADER_ZBIN32) ? STATE_HEADER_ZBIN32_CRC : STATE_HEADER_ZBIN_CRC);
            break;
        case STATE_HEADER_ZBIN_CRC:
            zstate.crc[zstate.count++] = byte;
            if (zstate.count >= 2) {
                uint16_t crc = zcrc16(zstate.header, 5, 0);
                uint16_t rxcrc = zstate.crc[1] | (zstate.crc[0] << 8);
                ZDEBUG("CRC calculated: %x, received %x\n", crc, rxcrc);
                if (crc == rxcrc) {
                    zmodem_process_header();
                } else {
                    zmodem_send_znak();
                    zmodem_enter_state(STATE_IDLE);
                }
            }
            break;
        case STATE_HEADER_ZBIN32_CRC:
            zstate.crc[zstate.count++] = byte;
            if (zstate.count >= 4) {
                uint32_t crc = zcrc32(zstate.header, 5, 0);
                uint32_t rxcrc = (uint32_t) zstate.crc[0] | ((uint32_t) zstate.crc[1] << 8) | ((uint32_t) zstate.crc[2] << 16) | ((uint32_t) zstate.crc[3] << 24);
                ZDEBUG("CRC calculated: %x, received %x\n", crc, rxcrc);
                if (crc == rxcrc) {
                    zmodem_process_header();
                } else {
                    zmodem_send_znak();
                    zmodem_enter_state(STATE_IDLE);
                }
            }
            break;
        case STATE_DATA_ZBIN:
        case STATE_DATA_ZBIN32:
#ifdef ZMODEM_O_BYTESIZE_WRITE
            if ((zstate.count < BUFFER_SIZE) && (zstate.protostate != PROTOSTATE_RX_TRANSFER)) {
#else
            if (zstate.count < BUFFER_SIZE) {
#endif
                zstate.buffer[zstate.count] = byte;
            }
            zstate.count++;
            if (zstate.zdlecount) {
                switch (byte) {
                case ZCRCE:
                    zstate.datalen = zstate.count - 1;
                    zstate.frametype = byte;
#if defined(ZMODEM_O_BYTESIZE_CRC32) || defined(ZMODEM_O_BYTESIZE_CRC16)
                    keep_byte = 0;
#endif
                    zmodem_enter_state((zstate.state ==
                                        STATE_DATA_ZBIN32) ? STATE_DATA_ZBIN32_ZCRCE : STATE_DATA_ZBIN_ZCRCE);
                    break;
                case ZCRCG:
                    zstate.datalen = zstate.count - 1;
                    zstate.frametype = byte;
#if defined(ZMODEM_O_BYTESIZE_CRC32) || defined(ZMODEM_O_BYTESIZE_CRC16)
                    keep_byte = 0;
#endif
                    zmodem_enter_state((zstate.state ==
                                        STATE_DATA_ZBIN32) ? STATE_DATA_ZBIN32_ZCRCG : STATE_DATA_ZBIN_ZCRCG);
                    break;
                case ZCRCQ:
                    zstate.datalen = zstate.count - 1;
                    zstate.frametype = byte;
#if defined(ZMODEM_O_BYTESIZE_CRC32) || defined(ZMODEM_O_BYTESIZE_CRC16)
                    keep_byte = 0;
#endif
                    zmodem_enter_state((zstate.state ==
                                        STATE_DATA_ZBIN32) ? STATE_DATA_ZBIN32_ZCRCQ : STATE_DATA_ZBIN_ZCRCQ);
                    break;
                case ZCRCW:
                    zstate.datalen = zstate.count - 1;
                    zstate.frametype = byte;
#if defined(ZMODEM_O_BYTESIZE_CRC32) || defined(ZMODEM_O_BYTESIZE_CRC16)
                    keep_byte = 0;
#endif
                    zmodem_enter_state((zstate.state ==
                                        STATE_DATA_ZBIN32) ? STATE_DATA_ZBIN32_ZCRCW : STATE_DATA_ZBIN_ZCRCW);
                    break;
                default:


                    break;
                }
            }
#if defined(ZMODEM_O_BYTESIZE_CRC32) || defined(ZMODEM_O_BYTESIZE_CRC16)
            if (keep_byte) {
#ifdef ZMODEM_O_BYTESIZE_WRITE
                if (zstate.protostate == PROTOSTATE_RX_TRANSFER) {
#ifdef HOST
                    write(fdout, &byte, 1); 
#else
                    // save byte to FRAM
                    fram_write_byte(byte);
#endif
                }
#endif
                if (zstate.state == STATE_DATA_ZBIN32) {
                    crc32bs_upd(byte);
                } else {
                    crc16bs_upd(byte);
                }
            }
#endif
            break;
        case STATE_DATA_ZBIN_ZCRCE:
        case STATE_DATA_ZBIN_ZCRCG:
        case STATE_DATA_ZBIN_ZCRCQ:
        case STATE_DATA_ZBIN_ZCRCW:
            zstate.crc[zstate.count++] = byte;
            if (zstate.count >= 2) {
#ifdef ZMODEM_O_BYTESIZE_CRC16
                crc16bs_upd(zstate.frametype);
                uint16_t crc = crc16bs_end();
#else
                zstate.buffer[zstate.datalen] = zstate.frametype;
                uint16_t crc = zcrc16(zstate.buffer, zstate.datalen+1, 0);
#endif
                uint16_t rxcrc = zstate.crc[1] | (zstate.crc[0] << 8);
                ZDEBUG("CRC is %x, received %x, len is %d\n", crc, rxcrc, zstate.datalen);
#ifdef __MSP430__
                if (crc == rxcrc) {
                    sig1_on;
                } else {
                    sig0_on;
                }
#endif
/*
                uint16_t i;
                ZDEBUG("checked the following:\r\n");
                for (i=0;i<zstate.datalen+1;i++) {
                    ZDEBUG(" %02x", zstate.buffer[i]);
                }
                ZDEBUG("\r\n");
*/
                zmodem_process_data(crc == rxcrc);
            }
            break;
        case STATE_DATA_ZBIN32_ZCRCE:
        case STATE_DATA_ZBIN32_ZCRCG:
        case STATE_DATA_ZBIN32_ZCRCQ:
        case STATE_DATA_ZBIN32_ZCRCW:
            zstate.crc[zstate.count++] = byte;
            if (zstate.count >= 4) {
#ifdef ZMODEM_O_BYTESIZE_CRC32
                crc32bs_upd(zstate.frametype);
                uint32_t crc = crc32bs_end();
#else
                zstate.buffer[zstate.datalen] = zstate.frametype;
                uint32_t crc = zcrc32(zstate.buffer, zstate.datalen+1, 0);
                //crc = crc32(&zstate.frametype, 1, crc);
#endif
                uint32_t rxcrc = (uint32_t) zstate.crc[0] | ((uint32_t) zstate.crc[1] << 8) | ((uint32_t) zstate.crc[2] << 16) | ((uint32_t) zstate.crc[3] << 24);
                ZDEBUG("CRC is %x, received %x, len is %d\n", crc, rxcrc, zstate.datalen);
#if 0
                uint16_t i;
                ZDEBUG("buffer was \r\n");
                for (i=0;i<zstate.datalen;i++) {
                    ZDEBUG("%x ", zstate.buffer[i]);
                    if ((i & 0xf) == 0xf) {
                        ZDEBUG("\r\n");
                    }
                }
                ZDEBUG("\r\n frametype %02x\r\n", zstate.frametype);
                //ZDEBUG("Buffer is %s\n", zstate.buffer);
#endif
#ifdef __MSP430__
                if (crc == rxcrc) {
                    sig1_on;
                } else {
                    sig0_on;
                }
#endif
                zmodem_process_data(crc == rxcrc);
            }
            break;
        case STATE_FINISH:
            if (byte == 'O')
                zstate.count++;
            else
                zstate.count = 0;
            if (zstate.count >= 2) {
                ZDEBUG("Received OO\n");
                zmodem_reinit();
            }
            break;
        default:
            zmodem_enter_state(STATE_IDLE);
            break;
        }
        zstate.zdlecount = 0;
    }
}

#ifdef HOST
#include <pty.h>
#include <string.h>

//FILE *term;

void ztx_byte(uint8_t byte)
{
//    ZDEBUG("Sending byte %x\n", byte);
    fputc(byte, stdout);
    fflush(stdout);
}

int main(void)
{
    int c;

#ifdef USE_PTY
    int master;
    int slave;
    char name[1024];
    struct termios termp;
    memset(&termp, 0, sizeof(termp));
    termp.c_iflag = IGNBRK | IGNPAR;
    termp.c_oflag = 0;
    termp.c_cflag = CS8 | CLOCAL;
    termp.c_lflag = 0;
    termp.c_cc[VMIN] = 1;
    termp.c_cc[VTIME] = 0;

    if (openpty(&master, &slave, name, &termp, NULL)) {
        printf("Can't open pty\n");
        return 1;
    }
    printf("pty is %s\n", name);

    term = fdopen(master, "r+b");
    setvbuf(term, NULL, _IONBF, 0);
    while ((c = fgetc(term)) != EOF)
        zrx_byte(c);
#else
    setbuf(stderr, NULL);
    while ((c = fgetc(stdin)) != EOF)
        zrx_byte(c);
#endif
    return 0;
}
#endif
