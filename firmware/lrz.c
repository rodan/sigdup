
#include "zglobal.h"

rz_sm_t rz_sm;

static struct zm_fileinfo zi;

enum zm_type_enum protocol;

uint8_t rzfile_sm()
{
    switch (rz_sm.state) {

    case RZ_INIT:
        Txhdr[ZF0] = CANFC32 | CANFDX | CANOVIO;
        Txhdr[ZF1] = 0;
        Txhdr[ZF2] = 0;
        Txhdr[ZF3] = 0;
        zshhdr(ZRINIT, Txhdr);  // 2a 2a 18 42 30 31 ..
        break;

    case RZ_INIT_2:
        Txhdr[ZF0] = CANFC32 | CANFDX | CANOVIO;
        Txhdr[ZF1] = 0;
        Txhdr[ZF2] = 0;
        Txhdr[ZF3] = 0;
        zshhdr(ZRINIT, Txhdr);  // 2a 2a 18 42 30 31 ..
        break;

    case RZ_POS:
        Txhdr[ZF0] = 0;
        Txhdr[ZF1] = 0;
        Txhdr[ZF2] = 0;
        Txhdr[ZF3] = 0;
        zshhdr(ZRPOS, Txhdr);   // 2a 2a 18 42 30 39 ..
        break;

    case RZ_RCV:
        break;

    case RZ_INIT_3:
        Txhdr[ZF0] = CANFC32 | CANFDX | CANOVIO;
        Txhdr[ZF1] = 0;
        Txhdr[ZF2] = 0;
        Txhdr[ZF3] = 0;
        zshhdr(ZRINIT, Txhdr);  // 2a 2a 18 42 30 31 ..
        break;

    case RZ_FIN:
        Txhdr[ZF0] = 0;
        Txhdr[ZF1] = 0;
        Txhdr[ZF2] = 0;
        Txhdr[ZF3] = 0;
        zshhdr(ZFIN, Txhdr);    // 2a 2a 18 42 30 38 ..
        break;

    default:
        return ERROR;
        break;
    }

    return OK;
}

void z_rst_zi(void)
{
    zi.fname = NULL;
    zi.modtime = 0;
    zi.mode = 0;
    zi.bytes_total = 0;
    zi.bytes_sent = 0;
    zi.bytes_received = 0;
    zi.bytes_skipped = 0;
    zi.eof_seen = 0;
}

int wcreceive(void)
{

    z_rst_zi();
    rz_sm.state = RZ_INIT;
    rzfile_sm();

    return OK;
}
