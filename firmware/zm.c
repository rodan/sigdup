/*
  zm.c - zmodem protocol handling lowlevelstuff
  Copyright (C) until 1998 Chuck Forsberg (OMEN Technology Inc)
  Copyright (C) 1996, 1997 Uwe Ohse

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
  02111-1307, USA.

  originally written by Chuck Forsberg
*/
/* historical comment: -- uwe
 *   Z M . C
 *    ZMODEM protocol primitives
 *    05-09-88  Chuck Forsberg Omen Technology Inc
 *
 * Entry point Functions:
 *	zsbhdr(type, hdr) send binary header
 *	zshhdr(type, hdr) send hex header
 *	zgethdr(hdr, eflag) receive header - binary or hex
 *	zsdata(buf, len, frameend) send data
 *	zrdata(buf, len, bytes_received) receive data
 *	stohdr(pos) store position data in Txhdr
 *	long rclhdr(hdr) recover position offset from header
 */

#include "zglobal.h"

#include <stdio.h>
#include "uart0.h"
#include "glue.h"

unsigned int Rxtimeout = 100;   /* Tenths of seconds to wait for something */

int Rxframeind;                 /* ZBIN ZBIN32, or ZHEX type of frame received */
int Rxtype;                     /* Type of header received */
char Txhdr[4];                  /* Transmitted header */
int Crc32t;                     /* Display flag indicating 32 bit CRC being sent */
int Crc32;                      /* Display flag indicating 32 bit CRC being received */

#if 0
static const char *frametypes[] = {
    "Carrier Lost",             /* -3 */
    "TIMEOUT",                  /* -2 */
    "ERROR",                    /* -1 */
#define FTOFFSET 3
    "ZRQINIT",
    "ZRINIT",
    "ZSINIT",
    "ZACK",
    "ZFILE",
    "ZSKIP",
    "ZNAK",
    "ZABORT",
    "ZFIN",
    "ZRPOS",
    "ZDATA",
    "ZEOF",
    "ZFERR",
    "ZCRC",
    "ZCHALLENGE",
    "ZCOMPL",
    "ZCAN",
    "ZFREECNT",
    "ZCOMMAND",
    "ZSTDERR",
    "xxxxx"
#define FRTYPES 22              /* Total number of frame types in this array */
        /*  not including psuedo negative entries */
};
#endif

static inline int noxrd7 __P((void));
static inline int zdlread __P((void));
static int zdlread2 __P((int)) LRZSZ_ATTRIB_REGPARM(1);
static inline int zgeth1 __P((void));
static void zputhex __P((int c, char *pos));
static inline int zgethex __P((void));
static int zrbhdr __P((char *hdr));
static int zrbhdr32 __P((char *hdr));
static int zrhhdr __P((char *hdr));

/*
 * Read a character from the modem line with timeout.
 *  Eat parity, XON and XOFF characters.
 */
static inline int noxrd7(void)
{
    register int c;
    //char itoa_buf[CONV_BASE_10_BUF_SZ];

    for (;;) {
        c = READLINE_PF(Rxtimeout);
        //uart0_print(_utoh(&itoa_buf[0], c));
        if (c < 0)
            return c;
        switch (c &= 0177) {
        case XON:
        case XOFF:
            continue;
        default:
            //if (Zctlesc && !(c & 0140))
            //      continue;
        case '\r':
        case '\n':
        case ZDLE:
            return c;
        }
    }
}

static inline int zgeth1(void)
{
    register int c, n;

    if ((c = noxrd7()) < 0)
        return c;
    n = c - '0';
    if (n > 9)
        n -= ('a' - ':');
    if (n & ~0xF)
        return ERROR;
    if ((c = noxrd7()) < 0)
        return c;
    c -= '0';
    if (c > 9)
        c -= ('a' - ':');
    if (c & ~0xF)
        return ERROR;
    c += (n << 4);
    return c;
}

/* Decode two lower case hex digits into an 8 bit byte value */
static inline int zgethex(void)
{
    register int c;

    c = zgeth1();
    //VPRINTF(9,("zgethex: %02X", c));
    return c;
}

/*
 * Read a byte, checking for ZMODEM escape encoding
 *  including CAN*5 which represents a quick abort
 */
static inline int zdlread(void)
{
    int c;
    /* Quick check for non control characters */
    if ((c = READLINE_PF(Rxtimeout)) & 0140)
        return c;
    return zdlread2(c);
}

/* no, i don't like gotos. -- uwe */
static int zdlread2(int c)
{
    goto jump_over;             /* bad style */

 again:
    /* Quick check for non control characters */
    if ((c = READLINE_PF(Rxtimeout)) & 0140)
        return c;
 jump_over:
    switch (c) {
    case ZDLE:
        break;
    case XON:
    case (XON | 0200):
    case XOFF:
    case (XOFF | 0200):
        goto again;
    default:
//              if (Zctlesc && !(c & 0140)) {
//                      goto again;
//              }
        return c;
    }
 again2:
    if ((c = READLINE_PF(Rxtimeout)) < 0)
        return c;
    if (c == CAN && (c = READLINE_PF(Rxtimeout)) < 0)
        return c;
    if (c == CAN && (c = READLINE_PF(Rxtimeout)) < 0)
        return c;
    if (c == CAN && (c = READLINE_PF(Rxtimeout)) < 0)
        return c;
    switch (c) {
    case CAN:
        return GOTCAN;
    case ZCRCE:
    case ZCRCG:
    case ZCRCQ:
    case ZCRCW:
        return (c | GOTOR);
    case ZRUB0:
        return 0177;
    case ZRUB1:
        return 0377;
    case XON:
    case (XON | 0200):
    case XOFF:
    case (XOFF | 0200):
        goto again2;
    default:
//              if (Zctlesc && ! (c & 0140)) {
//                      goto again2;
//              }
        if ((c & 0140) == 0100)
            return (c ^ 0100);
        break;
    }
//      VPRINTF(2,(_("Bad escape sequence %x"), c));
    return ERROR;
}

/* Send ZMODEM HEX header hdr of type type */
void zshhdr(int type, char *hdr)
{
    register int n;
    register unsigned short crc;
    char s[30];
    size_t len;

    s[0] = ZPAD;
    s[1] = ZPAD;
    s[2] = ZDLE;
    s[3] = ZHEX;
    zputhex(type & 0x7f, s + 4);
    len = 6;
    Crc32t = 0;

    crc = updcrc((type & 0x7f), 0);
    for (n = 4; --n >= 0; ++hdr) {
        zputhex(*hdr, s + len);
        len += 2;
        crc = updcrc((0377 & *hdr), crc);
    }
    crc = updcrc(0, updcrc(0, crc));
    zputhex(crc >> 8, s + len);
    zputhex(crc, s + len + 2);
    len += 4;

    /* Make it printable on remote machine */
    s[len++] = 015;
    s[len++] = 0212;
    /*
     * Uncork the remote in case a fake XOFF has stopped data flow
     */
    if (type != ZFIN && type != ZACK) {
        s[len++] = 021;
    }
    flushmo();
    uart0_tx_str(s, len);
}

void z_parse_frame(void)
{
    char *hdr = uart0_get_rx_buf();
    char itoa_buf[CONV_BASE_10_BUF_SZ];
    //uint8_t frame_type = 255;
    register int c;
    //size_t rxpos=0;

    Rxframeind = Rxtype = 0;

    //uart0_tx_str(&hdr[0],1);
    //uart0_tx_str(&hdr[1],1);
    //uart0_tx_str(&hdr[2],1);

    readline_ptr = uart0_get_rx_buf();
	readline_left = uart0_get_p();

    c = noxrd7();
    //uart0_print(_utoh(&itoa_buf[0], c));

    switch (c) {
    case RCDO:
    case TIMEOUT:
        //goto fifi;
        break;
    case ZBIN:
        Rxframeind = ZBIN;
        Crc32 = FALSE;
        c = zrbhdr(hdr);
        break;
    case ZBIN32:
        Crc32 = Rxframeind = ZBIN32;
        c = zrbhdr32(hdr);
        break;
    case ZHEX:
        Rxframeind = ZHEX;
        Crc32 = FALSE;
        c = zrhhdr(hdr);
        break;
    case CAN:
        //goto gotcan;
        break;
    default:
        //goto agn2;
        break;
    }

    //uart0_print(_utoh(&itoa_buf[0], c));

    switch (c) {
    case ZRQINIT:               // 0x0
        z_rst_zi();
        rz_sm.state = RZ_INIT_2;
        rzfile_sm();
        break;
    case ZFILE:                 // 0x4
        // need to populate zi
        rz_sm.state = RZ_POS;
        rzfile_sm();
        break;
    case ZDATA:                 // 0x10
        rz_sm.state = RZ_INIT_3;
        rzfile_sm();
        break;
    case ZFIN:                  // 0x8
        rz_sm.state = RZ_FIN;
        rzfile_sm();
        break;
    }

/*
    if (input[0] == ZHEX) {
        // hex frame indicator

        Rxframeind = ZHEX;  Crc32 = FALSE;
        frame_type = zrhhdr(hdr);
        // str_to_uint8(input, &frame_type, 1, 2, 0, 19);
        switch (frame_type) {
            case ZRQINIT:   // 0
                z_rst_zi();
                rz_sm.state = RZ_INIT_2;
                rzfile_sm();
                break;
            case ZFIN:      // 8
                break;
        }
        //uart0_print(_utoh(&itoa_buf[0], frame_type));
    } else if (input[0] == ZBIN32) {
        // Binary frame with 32 bit FCS
        
    }

*/
    // foo
}

/* Receive a binary style header (type and position) */
static int zrbhdr(char *hdr)
{
    register int c, n;
    register unsigned short crc;

    if ((c = zdlread()) & ~0377)
        return c;
    Rxtype = c;
    crc = updcrc(c, 0);

    for (n = 4; --n >= 0; ++hdr) {
        if ((c = zdlread()) & ~0377)
            return c;
        crc = updcrc(c, crc);
        *hdr = c;
    }
    if ((c = zdlread()) & ~0377)
        return c;
    crc = updcrc(c, crc);
    if ((c = zdlread()) & ~0377)
        return c;
    crc = updcrc(c, crc);
    if (crc & 0xFFFF) {
        //zperr(badcrc); 
        return ERROR;
    }
    protocol = ZM_ZMODEM;
    return Rxtype;
}

/* Receive a binary style header (type and position) with 32 bit FCS */
static int zrbhdr32(char *hdr)
{
    register int c, n;
    register unsigned long crc;
    char itoa_buf[CONV_BASE_10_BUF_SZ];

    if ((c = zdlread()) & ~0377) {
        return c;
    }
    Rxtype = c;

    // if this is data, we need to test the crc in other ways
    if (Rxtype == ZDATA) {
        return Rxtype;
    }

    crc = 0xFFFFFFFFL;
    crc = UPDC32(c, crc);
        
    //uart0_print(_utoh(&itoa_buf[0], c));
#ifdef DEBUGZ
    VPRINTF(3, ("zrbhdr32 c=%X  crc=%lX", c, crc) i);
#endif

    for (n = 4; --n >= 0; ++hdr) {
        if ((c = zdlread()) & ~0377)
            return c;
        crc = UPDC32(c, crc);
        *hdr = c;
#ifdef DEBUGZ
        VPRINTF(3, ("zrbhdr32 c=%X  crc=%lX", c, crc));
#endif
    }
    for (n = 4; --n >= 0;) {
        if ((c = zdlread()) & ~0377)
            return c;
        crc = UPDC32(c, crc);
#ifdef DEBUGZ
        VPRINTF(3, ("zrbhdr32 c=%X  crc=%lX", c, crc));
#endif
    }
    if (crc != 0xDEBB20E3) {
        //zperr(badcrc);
        return ERROR;
    }
    protocol = ZM_ZMODEM;
    return Rxtype;
}

/* Receive a hex style header (type and position) */
static int zrhhdr(char *hdr)
{
    register int c;
    register unsigned short crc;
    register int n;

    if ((c = zgethex()) < 0)
        return c;
    Rxtype = c;
    crc = updcrc(c, 0);

    for (n = 4; --n >= 0; ++hdr) {
        if ((c = zgethex()) < 0)
            return c;
        crc = updcrc(c, crc);
        *hdr = c;
    }
    if ((c = zgethex()) < 0)
        return c;
    crc = updcrc(c, crc);
    if ((c = zgethex()) < 0)
        return c;
    crc = updcrc(c, crc);
    if (crc & 0xFFFF) {
        //zperr(badcrc); 
        return ERROR;
    }
    switch (c = READLINE_PF(1)) {
    case 0215:
        /* **** FALL THRU TO **** */
    case 015:
        /* Throw away possible cr/lf */
        READLINE_PF(1);
        break;
    }
    protocol = ZM_ZMODEM;
    return Rxtype;
}

/* Write a byte as two hex digits */
static void zputhex(int c, char *pos)
{
    static char digits[] = "0123456789abcdef";

    pos[0] = digits[(c & 0xF0) >> 4];
    pos[1] = digits[c & 0x0F];
}
