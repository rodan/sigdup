
#include <msp430.h>
#include <inttypes.h>
#include <string.h>
#include "driverlib.h"
#include "zmodem.h"
#include "atlas430.h"
#include "uart0_extras.h"
#include "timer_a0.h"
#include "sig.h"
#include "zmodem.h"

volatile uint8_t uart0_input_type;      // type of input - either user generated or zmodem

uint8_t uart0_get_input_type(void)
{
    return uart0_input_type;
}

void uart0_set_input_type(const uint8_t type)
{
    uart0_input_type = type;
}

uint8_t uart0_extra_irq_handler(uart_descriptor *uartd, const uint8_t c)
{
    uint8_t ret_type = UART_RET_SLEEP;

//#ifdef LED_SYSTEM_STATES
    sig2_on;
//#endif

    if (uart0_input_type == RX_USER) {
        if (c == 0x0a) {
//#ifdef LED_SYSTEM_STATES
            sig2_off;
//#endif
            return UART_RET_SLEEP;
        }

        if (!ringbuf_put(&uartd->rbrx, c)) {
            // the ringbuffer is full
            sig3_switch;
            uartd->rx_err++;
            ret_type = UART_RET_WAKE;
        }

        if (c == 0x0d) {
            return UART_RET_WAKE;
        } else if (c == ZPAD) {
            uart0_input_type = RX_ZMODEM_HDR;
            ret_type = UART_RET_WAKE;
            zmodem_init();
        }

    } else if (uart0_input_type == RX_ZMODEM_HDR) {
        // zmodem header frames
        if (!ringbuf_put(&uartd->rbrx, c)) {
            // the ringbuffer is full
            uartd->rx_err++;
        }

        ret_type = UART_RET_WAKE;
        timer_a0_delay_noblk_ccr1(intrchar_tmout);
    }

//#ifdef LED_SYSTEM_STATES
    sig2_off;
//#endif

    return ret_type;
}
