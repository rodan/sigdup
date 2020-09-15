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
#include "glue.h"

char Txhdr[4];		/* Transmitted header */
int Crc32t;		/* Display flag indicating 32 bit CRC being sent */

static const char *frametypes[] = {
	"Carrier Lost",		/* -3 */
	"TIMEOUT",		/* -2 */
	"ERROR",		/* -1 */
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
#define FRTYPES 22	/* Total number of frame types in this array */
			/*  not including psuedo negative entries */
};

static void zputhex __P ((int c, char *pos));



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

/* Write a byte as two hex digits */
static void 
zputhex(int c, char *pos)
{
	static char	digits[] = "0123456789abcdef";

	pos[0]=digits[(c&0xF0)>>4];
	pos[1]=digits[c&0x0F];
}

