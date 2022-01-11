
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

#include "proj.h"
#include "glue.h"
#include "timer_a2.h"
#include "ui.h"
#include "version.h"
#include "zmodem.h"
#include "fram_glue.h"
#include "timer_a1.h"
#include "pg.h"
#include "sig.h"

extern const uint32_t HIGH_FRAM_START;
extern uart_descriptor bc;

void display_menu(void)
{
    char itoa_buf[CONV_BASE_10_BUF_SZ];
    uart_print(&bc, "\r\nsigdup build ");
    uart_print(&bc, _utoa(&itoa_buf[0], BUILD));
    uart_print(&bc, "\r\n\r\ngeneric commands:\r\n\r\n");
    uart_print(&bc, " \e[33;1m?\e[0m    - show help\r\n");
    //uart_print(&bc, " \e[33;1mpa\e[0m   - parse HIFRAM\r\n");
    //uart_print(&bc, " \e[33;1mprep\e[0m - prepare signal\r\n");
    uart_print(&bc, " \e[33;1mgo\e[0m   - replay signal\r\n");
    uart_print(&bc, " \e[33;1mtest\e[0m - generate test signal\r\n");
    uart_print(&bc, " \e[33;1mread\e[0m - read HIFRAM\r\n");
    uart_print(&bc, " \e[33;1mpeek\e[0m - peek HIFRAM\r\n");
    uart_print(&bc, " \e[33;1msch\e[0m  - show schedule\r\n");
}

void display_schedule(void)
{
    uint8_t c;
    uint32_t trigger;
    uint8_t flag;
    char itoa_buf[CONV_BASE_10_BUF_SZ];

    for (c = 0; c < TIMER_A2_SLOTS_COUNT; c++) {
        timer_a2_get_trigger_slot(c, &trigger, &flag);
        uart_print(&bc, _utoa(itoa_buf, c));
        uart_print(&bc, " \t");
        uart_print(&bc, _utoa(itoa_buf, trigger));
        uart_print(&bc, " \t");
        uart_print(&bc, _utoa(itoa_buf, flag));
        uart_print(&bc, "\r\n");
    }
    trigger = timer_a2_get_trigger_next();
    uart_print(&bc, "sch next ");
    uart_print(&bc, _utoa(itoa_buf, trigger));
    uart_print(&bc, "\r\n");
}

void create_test_sig(void)
{
    const uint16_t pkt_cnt = 1004;
    uint16_t c;

    fram_header fram_hdr;
    replay_header_t replay_hdr;
    replay_packet_8ch_t pkt;

    fram_hdr.file_sz = sizeof(replay_hdr) + (pkt_cnt * sizeof(replay_packet_8ch_t));

    replay_hdr.version = VER_REPLAY;
    replay_hdr.header_size = sizeof(replay_hdr);
    replay_hdr.packet_count = pkt_cnt;
    replay_hdr.bytes_per_packet = sizeof(replay_packet_8ch_t);
    replay_hdr.block_size = 1;
    replay_hdr.clk_divider = CLK_DIV_1;

    fram_init();
    fram_write_header(&fram_hdr);
    fram_write((uint8_t *)&replay_hdr, sizeof(replay_header_t));

    pkt.ccr = 0;

    pkt.sig = 0xf0;
    pkt.ccr += 1000;
    fram_write((uint8_t *)&pkt, sizeof(pkt));
    pkt.sig = 0x0f;
    pkt.ccr += 1000;
    fram_write((uint8_t *)&pkt, sizeof(pkt));
    
    for (c = 0; c < 1000; c++) {
        pkt.sig = c;
        pkt.ccr += 160; // 50kHz
        fram_write((uint8_t *)&pkt, sizeof(pkt));
    }

    pkt.sig = 0xf0;
    pkt.ccr += 1000;
    fram_write((uint8_t *)&pkt, sizeof(pkt));
    pkt.sig = 0x00;
    pkt.ccr += 1000;
    fram_write((uint8_t *)&pkt, sizeof(pkt));

    prepare_signal();
}

//#define USE_STRUCT_POSITIONING
//#define USE_POINTER

void parse_fram(void)
{
    char itoa_buf[CONV_BASE_10_BUF_SZ];

    uint32_t test;
    uint32_t stream_pos;   /// FRAM address for the next stream data pkt
    uint32_t stream_start; /// FRAM address where the stream starts
    uint32_t stream_end;   /// FRAM address where the stream ends

    uint8_t next_sig = 0;
    uint16_t next_ccr = 0;

    replay_packet_8ch_t *pkt_8ch;

    stream_start = timer_a1_get_stream_start();
    stream_end = timer_a1_get_stream_end();
    stream_pos = stream_start;

    test = 0;

    sig0_on;
    while (stream_pos < stream_end) {

        pkt_8ch = (replay_packet_8ch_t *) stream_pos;
        next_sig = pkt_8ch->sig;
        next_ccr = pkt_8ch->ccr;

        //uart_print(&bc, " sig ");
        //uart_print(&bc, _utoh(itoa_buf, next_sig));
        //uart_print(&bc, " ccr ");
        //uart_print(&bc, _utoa(itoa_buf, next_ccr));
        //uart_print(&bc, "\r\n");

        test += next_sig + next_ccr;

        stream_pos += 3;
    }
    sig0_off;

    uart_print(&bc, _utoa(itoa_buf, test));
}

void print_buf(uint8_t * data, const uint16_t size)
{
    uint16_t bytes_remaining = size;
    uint16_t bytes_to_be_printed, bytes_printed = 0;
    char itoa_buf[CONV_BASE_10_BUF_SZ];
    uint16_t i;

    while (bytes_remaining > 0) {

        if (bytes_remaining > 16) {
            bytes_to_be_printed = 16;
        } else {
            bytes_to_be_printed = bytes_remaining;
        }

        uart_print(&bc, _utoh16(itoa_buf, bytes_printed));
        uart_print(&bc, ": ");

        for (i = 0; i < bytes_to_be_printed; i++) {
            uart_print(&bc, _utoh8(itoa_buf, data[bytes_printed + i]));
            if (i & 0x1) {
                uart_print(&bc, " ");
            }
        }

        uart_print(&bc, "\r\n");
        bytes_printed += bytes_to_be_printed;
        bytes_remaining -= bytes_to_be_printed;
    }
}

void print_buf_fram(const uint32_t address, const uint32_t size)
{
    uint32_t bytes_remaining = size;
    uint32_t bytes_to_be_printed, bytes_printed = 0;
    char itoa_buf[CONV_BASE_10_BUF_SZ];
    uint16_t i;
    uint8_t *read_ptr = (uint8_t *) address;

    while (bytes_remaining > 0) {

        if (bytes_remaining > 16) {
            bytes_to_be_printed = 16;
        } else {
            bytes_to_be_printed = bytes_remaining;
        }

        uart_print(&bc, _utoh32(itoa_buf, bytes_printed));
        uart_print(&bc, ": ");

        for (i = 0; i < bytes_to_be_printed; i++) {
            uart_print(&bc, _utoh8(itoa_buf, *read_ptr++));
            if (i & 0x1) {
                uart_print(&bc, " ");
            }
        }

        uart_print(&bc, "\r\n");
        bytes_printed += bytes_to_be_printed;
        bytes_remaining -= bytes_to_be_printed;
    }
}

void print_buf_ascii2mem(uint8_t * data, const uint16_t size)
{
    uint8_t byte;
    uint8_t nib;
    uint8_t data_out;
    uint16_t len = size / 2;

    while (len) {
        byte = *(data++);
        if (byte < 0x40) {
            byte -= 48;
        } else {
            byte -= 55;
        }
        nib = byte << 4;

        byte = *(data++);
        if (byte < 0x40) {
            byte -= 48;
        } else {
            byte -= 55;
        }

        data_out = byte | nib;

        uart_tx_str(&bc, (char *)&data_out, 1);

        len--;
    }

    uart_print(&bc, "\r\n");
}

void print_buf_array(uint8_t * data, const uint16_t size)
{
    uint16_t bytes_remaining = size;
    uint16_t bytes_to_be_printed, bytes_printed = 0;
    char itoa_buf[CONV_BASE_10_BUF_SZ];
    uint16_t i;

    while (bytes_remaining > 0) {

        if (bytes_remaining > 8) {
            bytes_to_be_printed = 8;
        } else {
            bytes_to_be_printed = bytes_remaining;
        }

        for (i = 0; i < bytes_to_be_printed; i++) {
            uart_print(&bc, _utoh(itoa_buf, data[bytes_printed + i]));
            uart_print(&bc, ", ");
        }

        uart_print(&bc, "\r\n");
        bytes_printed += bytes_to_be_printed;
        bytes_remaining -= bytes_to_be_printed;
    }
}

void print_buf_native(uint8_t * data, const uint16_t size)
{
    uart_tx_str(&bc, (char *)data, size);
    uart_print(&bc, "\r\n");
}

#define PARSER_CNT 8

void parse_user_input(void)
{

#if defined UART_RX_USES_RINGBUF
    struct ringbuf *rbr = uart_get_rx_ringbuf(&bc);
    uint8_t rx;
    uint8_t c = 0;
    char input[PARSER_CNT];

    memset(input, 0, PARSER_CNT);

    // read the entire ringbuffer
    while (ringbuf_get(rbr, &rx)) {
        if (c < PARSER_CNT-1) {
            input[c] = rx;
        }
        c++;
    }
#else
    char *input = uart_get_rx_buf(&bc);
#endif
    char f = input[0];
    //uint16_t u16;
    //uint16_t i;
    //char itoa_buf[CONV_BASE_10_BUF_SZ];
    fram_header *hdr;

    if (f == '?') {
        display_menu();
    } else if (strstr(input, "sch")) {
        display_schedule();
    } else if (strstr(input, "peek")) {
        print_buf_fram(HIGH_FRAM_ADDR, 256);
//    } else if (strstr(input, "pa")) {
//        parse_fram();
//    } else if (strstr(input, "prep")) {
//        prepare_signal();
    } else if (strstr(input, "go")) {
        replay_signal();
    } else if (strstr(input, "test")) {
        create_test_sig();
    } else if (strstr(input, "read")) {
        hdr = (fram_header *) (uintptr_t) HIGH_FRAM_ADDR;
        if (hdr->file_start == HIGH_FRAM_START + sizeof(fram_header)) {
            print_buf_fram(hdr->file_start, hdr->file_sz);
        } else {
            uart_print(&bc, "no file present\r\n");
        }
    }
}
