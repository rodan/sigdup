
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

#include "glue.h"
#include "timer_a2.h"
#include "uart0.h"
#include "ui.h"
#include "version.h"
#include "zmodem.h"

void display_menu(void)
{
    char itoa_buf[CONV_BASE_10_BUF_SZ];
    uart0_print("\r\nsigdup build ");
    uart0_print(_utoa(&itoa_buf[0], BUILD));
    uart0_print("\r\n\r\ngeneric commands:\r\n\r\n");
    uart0_print(" \e[33;1m?\e[0m    - show help\r\n");
    uart0_print(" \e[33;1msch\e[0m  - show schedule\r\n");
    uart0_print(" \e[33;1mread\e[0m - read HIFRAM\r\n");
    uart0_print(" \e[33;1mgo\e[0m   - start transfer\r\n");
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

void print_buf_ascii2mem(uint8_t * data, const uint16_t size)
{
    uint8_t byte;
    uint8_t nib;
    uint8_t data_out;
    uint16_t len = size/2;

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

void parse_user_input(void)
{
    char *input = uart0_get_rx_buf();
    char f = input[0];
    //uint16_t u16;
    //uint16_t i;
    //char itoa_buf[CONV_BASE_10_BUF_SZ];

    if (f == '?') {
        display_menu();
    } else if (strstr(input, "sch")) {
        display_schedule();
    } else if (strstr(input, "read")) {
        print_buf((uint8_t *)(uintptr_t) HIGH_FRAM_ADDR, 512);
    } else if (strstr(input, "go")) {
        uart0_print("use zmodem to send file\r\n");
        //wcreceive();
        zmodem_init();
        uart0_set_input_type(RX_ZMODEM_HDR);
    }
}

