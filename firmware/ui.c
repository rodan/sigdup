
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

#include "proj.h"
#include "glue.h"
#include "timer_a2.h"
#include "uart0.h"
#include "ui.h"
#include "version.h"
#include "zmodem.h"
#include "fram_glue.h"
#include "timer_a1.h"
#include "pg.h"

extern const uint32_t HIGH_FRAM_START;

void display_menu(void)
{
    char itoa_buf[CONV_BASE_10_BUF_SZ];
    uart0_print("\r\nsigdup build ");
    uart0_print(_utoa(&itoa_buf[0], BUILD));
    uart0_print("\r\n\r\ngeneric commands:\r\n\r\n");
    uart0_print(" \e[33;1m?\e[0m    - show help\r\n");
    uart0_print(" \e[33;1msch\e[0m  - show schedule\r\n");
    uart0_print(" \e[33;1mread\e[0m - read HIFRAM\r\n");
    uart0_print(" \e[33;1mpeek\e[0m - peek HIFRAM\r\n");
    uart0_print(" \e[33;1mpa\e[0m   - parse HIFRAM\r\n");
    uart0_print(" \e[33;1mtest\e[0m - generate test signal\r\n");
    uart0_print(" \e[33;1mprep\e[0m - prepare signal\r\n");
    uart0_print(" \e[33;1mgo\e[0m   - replay signal\r\n");
}

void display_DONE(void)
{
    uart0_print("done\r\n");
}

void display_SEP(void)
{
    uart0_print("---------------\r\n");
}

void display_PASSED(void)
{
    uart0_print("\e[32;1mpassed\e[0m\r\n");
}

void display_FAILED(void)
{
    uart0_print("\e[31;1mfailed\e[0m\r\n");
}

void display_schedule(void)
{
    uint8_t c;
    uint32_t trigger;
    uint8_t flag;
    char itoa_buf[CONV_BASE_10_BUF_SZ];

    for (c = 0; c < TIMER_A2_SLOTS_COUNT; c++) {
        timer_a2_get_trigger_slot(c, &trigger, &flag);
        uart0_print(_utoa(itoa_buf, c));
        uart0_print(" \t");
        uart0_print(_utoa(itoa_buf, trigger));
        uart0_print(" \t");
        uart0_print(_utoa(itoa_buf, flag));
        uart0_print("\r\n");
    }
    trigger = timer_a2_get_trigger_next();
    uart0_print("sch next ");
    uart0_print(_utoa(itoa_buf, trigger));
    uart0_print("\r\n");
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
        pkt.ccr += 184;
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

void parse_fram(void)
{
    char itoa_buf[CONV_BASE_10_BUF_SZ];

    volatile uint32_t stream_pos;   /// FRAM address for the next stream data pkt
    volatile uint32_t stream_start; /// FRAM address where the stream starts
    volatile uint32_t stream_end;   /// FRAM address where the stream ends

    volatile uint8_t next_sig = 0;
    volatile uint16_t next_ccr = 0;

    replay_packet_8ch_t *pkt_8ch;

    stream_start = timer_a1_get_stream_start();
    stream_end = timer_a1_get_stream_end();
    stream_pos = stream_start;

    while (stream_pos < stream_end) {
        pkt_8ch = (replay_packet_8ch_t *) stream_pos;
        next_sig = pkt_8ch->sig;
        next_ccr = pkt_8ch->ccr;

        //next_sig = *((uint8_t *) stream_pos);
        //next_ccr = *((uint16_t *) (stream_pos + 1));

        uart0_print(" sig ");
        uart0_print(_utoh(itoa_buf, next_sig));
        uart0_print(" ccr ");
        uart0_print(_utoa(itoa_buf, next_ccr));
        uart0_print("\r\n");

        stream_pos += 3;
    }
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

        uart0_print(_utoh16(itoa_buf, bytes_printed));
        uart0_print(": ");

        for (i = 0; i < bytes_to_be_printed; i++) {
            uart0_print(_utoh8(itoa_buf, data[bytes_printed + i]));
            if (i & 0x1) {
                uart0_print(" ");
            }
        }

        uart0_print("\r\n");
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

        uart0_print(_utoh32(itoa_buf, bytes_printed));
        uart0_print(": ");

        for (i = 0; i < bytes_to_be_printed; i++) {
            uart0_print(_utoh8(itoa_buf, *read_ptr++));
            if (i & 0x1) {
                uart0_print(" ");
            }
        }

        uart0_print("\r\n");
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

        uart0_tx_str((char *)&data_out, 1);

        len--;
    }

    uart0_print("\r\n");
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
            uart0_print(_utoh(itoa_buf, data[bytes_printed + i]));
            uart0_print(", ");
        }

        uart0_print("\r\n");
        bytes_printed += bytes_to_be_printed;
        bytes_remaining -= bytes_to_be_printed;
    }
}

void print_buf_native(uint8_t * data, const uint16_t size)
{
    uart0_tx_str((char *)data, size);
    uart0_print("\r\n");
}

#define PARSER_CNT 8

void parse_user_input(void)
{

#ifdef UART0_RX_USES_RINGBUF
    //struct ringbuf *rbrx = uart0_get_rx_ringbuf();
    uint8_t rx;
    uint8_t c = 0;
    char input[PARSER_CNT];

    memset(input, 0, PARSER_CNT);

    // read the entire ringbuffer
    while (ringbuf_get(&uart0_rbrx, &rx)) {
        if (c < PARSER_CNT - 1) {
            input[c] = rx;
        }
        c++;
    }
#else
    char *input = uart0_get_rx_buf();
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
    } else if (strstr(input, "pa")) {
        parse_fram();
    } else if (strstr(input, "prep")) {
        prepare_signal();
    } else if (strstr(input, "go")) {
        replay_signal();
    } else if (strstr(input, "test")) {
        create_test_sig();
    } else if (strstr(input, "read")) {
        hdr = (fram_header *) (uintptr_t) HIGH_FRAM_ADDR;
        if (hdr->file_start == HIGH_FRAM_START + sizeof(fram_header)) {
            print_buf_fram(hdr->file_start, hdr->file_sz);
        } else {
            uart0_print("no file present\r\n");
        }
    }
}
