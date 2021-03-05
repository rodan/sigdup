
#include <msp430.h>
#include <inttypes.h>
#include <string.h>
#include "driverlib.h"
#include "zmodem.h"
#include "uart0.h"
#include "timer_a0.h"

// get the UART0_SPEED_ #define
#include "config.h"
#include "zmodem.h"

volatile char uart0_rx_buf[UART0_RXBUF_SZ];     // receive buffer
static volatile uint8_t uart0_p;        // number of characters received, 0 if none
static volatile uint8_t uart0_rx_enable;
static volatile uint8_t uart0_rx_err;

volatile uint8_t uart0_last_event;
volatile uint16_t intrchar_tmout;

volatile uint32_t f_addr_cur;
volatile uint32_t f_counter;

volatile uint8_t uart0_input_type;       // type of input - either user generated or zmodem (see input_type enum below)

// you'll have to initialize/map uart ports in main()
// or use uart0_port_init() if no mapping is needed
void uart0_init(void)
{
    UCA0CTLW0 = UCSWRST;        // put eUSCI state machine in reset

    // consult 'Recommended Settings for Typical Crystals and Baud Rates' in slau367o
    // for some reason any baud >= 115200 ends up with a non-working RX channel

#if defined(CPU_FREQ_1M)

#if defined (UART0_SPEED_9600)
    UCA0CTLW0 |= UCSSEL__SMCLK;
    UCA0BRW = 6;
    UCA0MCTLW = 0x2081;
#elif defined (UART0_SPEED_19200)
    UCA0CTLW0 |= UCSSEL__SMCLK;
    UCA0BRW = 3;
    UCA0MCTLW = 0x0241;
#elif defined (UART0_SPEED_38400)
    UCA0CTLW0 |= UCSSEL__SMCLK;
    UCA0BRW = 1;
    UCA0MCTLW = 0x00a1;
#elif defined (UART0_SPEED_57600)
    UCA0CTLW0 |= UCSSEL__SMCLK;
    UCA0BRW = 17;
    UCA0MCTLW = 0x4a00;
#elif defined (UART0_SPEED_115200)
    UCA0CTLW0 |= UCSSEL__SMCLK;
    UCA0BRW = 8;
    UCA0MCTLW = 0xd600;
#endif

#elif defined (CPU_FREQ_8M)

#if defined (UART0_SPEED_9600)
    UCA0CTLW0 |= UCSSEL__SMCLK;
    UCA0BRW = 52;
    UCA0MCTLW = 0x4911;
#elif defined (UART0_SPEED_19200)
    UCA0CTLW0 |= UCSSEL__SMCLK;
    UCA0BRW = 26;
    UCA0MCTLW = 0xb601;
#elif defined (UART0_SPEED_38400)
    UCA0CTLW0 |= UCSSEL__SMCLK;
    UCA0BRW = 13;
    UCA0MCTLW = 0x8401;
#elif defined (UART0_SPEED_57600)
    UCA0CTLW0 |= UCSSEL__SMCLK;
    UCA0BRW = 8;
    UCA0MCTLW = 0xf7a1;
#elif defined (UART0_SPEED_115200)
    UCA0CTLW0 |= UCSSEL__SMCLK;
    UCA0BRW = 4;
    UCA0MCTLW = 0x5551;
#endif

#elif defined (CPU_FREQ_16M)

#if defined (UART0_SPEED_9600)
    UCA0CTLW0 |= UCSSEL__SMCLK;
    UCA0BRW = 104;
    UCA0MCTLW = 0xd621;
#elif defined (UART0_SPEED_57600)
    UCA0CTLW0 |= UCSSEL__SMCLK;
    UCA0BRW = 17;
    UCA0MCTLW = 0xdd51;
#endif

#else                           // a safer default of 9600 - does not depend on SMCLK
    UCA0CTLW0 |= UCSSEL__ACLK;
    UCA0BRW = 3;
    //UCA0MCTLW |= 0x5300;
    UCA0MCTLW |= 0x9200;
#endif

    UCA0CTLW0 &= ~UCSWRST;      // Initialize eUSCI
    UCA0IE |= UCRXIE;           // Enable USCI_A0 RX interrupt

    uart0_p = 0;
    uart0_rx_enable = 1;
    uart0_rx_err = 0;
    uart0_input_type = RX_USER;
    intrchar_tmout = 500;       // _2ms
}

void uart0_initb(const uint8_t baudrate)
{
    UCA3CTLW0 = UCSWRST;        // put eUSCI state machine in reset

    // consult 'Recommended Settings for Typical Crystals and Baud Rates' in slau367o
    // for some reason any baud >= 115200 ends up with a non-working RX channel

    switch (baudrate) {
    case BAUDRATE_9600:
        UCA0CTLW0 |= UCSSEL__SMCLK;
        UCA0BRW = 52;
        UCA0MCTLW = 0x4911;
        break;
    case BAUDRATE_19200:
        UCA0CTLW0 |= UCSSEL__SMCLK;
        UCA0BRW = 26;
        UCA0MCTLW = 0xb601;
        break;
    case BAUDRATE_57600:
        UCA0CTLW0 |= UCSSEL__SMCLK;
        UCA0BRW = 8;
        UCA0MCTLW = 0xf7a1;
        break;
    }

    UCA0CTLW0 &= ~UCSWRST;      // Initialize eUSCI
    UCA0IE |= UCRXIE;           // Enable USCI_A3 RX interrupt

    uart0_p = 0;
    uart0_rx_enable = 1;
    uart0_rx_err = 0;
}

// default port locations
void uart0_port_init(void)
{
    P2SEL0 &= ~(BIT0 | BIT1);
    P2SEL1 |= (BIT0 | BIT1);
}

uint8_t uart0_get_event(void)
{
    return uart0_last_event;
}

void uart0_rst_event(void)
{
    uart0_last_event = UART0_EV_NULL;
}

void uart0_set_eol(void)
{
    uart0_p = 0;
    uart0_rx_enable = 1;
    sig3_off;
}

char *uart0_get_rx_buf(void)
{
    if (uart0_p) {
        return (char *)uart0_rx_buf;
    } else {
        return NULL;
    }
}

uint8_t uart0_get_p(void)
{
    return uart0_p;
}

void uart0_tx(const uint8_t byte)
{
    while (!(UCA0IFG & UCTXIFG)) {
    }                       // USCI_A0 TX buffer ready?
    UCA0TXBUF = byte;
}

uint16_t uart0_tx_str(const char *str, const uint16_t size)
{
    uint16_t p = 0;
    while (p < size) {
        while (!(UCA0IFG & UCTXIFG)) {
        }                       // USCI_A0 TX buffer ready?
        UCA0TXBUF = str[p];
        p++;
    }
    return p;
}

uint16_t uart0_print(const char *str)
{
    size_t p = 0;
    size_t size = strlen(str);
    while (p < size) {
        while (!(UCA0IFG & UCTXIFG)) {
        }                       // USCI_A0 TX buffer ready?
        UCA0TXBUF = str[p];
        p++;
    }
    return p;
}

void uart0_disable_rx(void)
{
    uart0_rx_enable = 0;
}

uint8_t uart0_get_input_type(void) {
    return uart0_input_type;
}

void uart0_set_input_type(const uint8_t type) {
    uart0_input_type = type;
}


#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=EUSCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(EUSCI_A0_VECTOR))) USCI_A0_ISR(void)
#else
#error Compiler not supported!
#endif
{
    uint16_t iv = UCA0IV;
    register char rx;
    //uint8_t c;
    uint8_t ev = 0;

#ifdef LED_SYSTEM_STATES
    sig3_on;
#endif

    switch (iv) {
    case USCI_UART_UCRXIFG:
        rx = UCA0RXBUF;

        if (uart0_input_type == RX_USER) {
            if (rx == 0x0a) {
                return;
            }

            if (uart0_p == 2) {
                if ((rx == ZDLE) && (uart0_rx_buf[0] == ZPAD) && (uart0_rx_buf[1] == ZPAD)) {
                    // ZPAD ZPAD ZDLE signals the start of a zmodem header
                    //sig4_on;
                    uart0_p++;
                    uart0_rx_buf[2] = rx;
                    zmodem_init();
                    uart0_input_type = RX_ZMODEM_HDR;
                    ev = UART0_EV_RX;
                    _BIC_SR_IRQ(LPM3_bits);
                    return;
                }
            }

            if (uart0_rx_enable && (!uart0_rx_err) && (uart0_p < UART0_RXBUF_SZ)) {
                if (rx == 0x0d) {
                    sig3_off;
                    uart0_rx_buf[uart0_p] = 0;
                    uart0_rx_enable = 0;
                    uart0_rx_err = 0;
                    //if (uart0_p) {
                    ev = UART0_EV_RX;
                    _BIC_SR_IRQ(LPM3_bits);
                    //}
                } else {
                    sig3_on;
                    uart0_rx_buf[uart0_p] = rx;
                    uart0_p++;
                }
            } else {
                uart0_rx_err++;
                uart0_p = 0;
                sig3_off;
                if ((rx == 0x0d) || (rx == 0x0a)) {
                    uart0_rx_err = 0;
                    uart0_rx_enable = 1;
                }
            }
        } else if (uart0_input_type == RX_ZMODEM_HDR) {
            //sig4_on;
            // zmodem header frames
            if (uart0_rx_enable && (uart0_p < UART0_RXBUF_SZ)) {
                uart0_rx_buf[uart0_p] = rx;
                uart0_rx_buf[uart0_p + 1] = 0;
                uart0_p++;
                ev = UART0_EV_RX;
                _BIC_SR_IRQ(LPM3_bits);
                timer_a0_delay_noblk_ccr1(intrchar_tmout);
            }
            /*
            if ((uart0_p == 10) && (uart0_rx_buf[0] == ZBIN32) && (uart0_rx_buf[1] == ZDATA)) {
                sig4_on;
                //uart0_input_type = RX_ZMODEM_DATA;
                //f_counter = 0;
                //f_addr_cur = HIGH_FRAM_ADDR;
                return;
            }
        } else if (uart0_input_type == RX_ZMODEM_DATA) {
            // zmodem data - needs to end up in hifram
            //c = rx;
            //FRAMCtl_A_write8(&c, (uint8_t *)f_addr_cur++, 1);
            //f_addr_cur++;
            //f_counter++;
            //timer_a0_delay_noblk_ccr1(intrchar_tmout);
        */
        }
        break;
    case USCI_UART_UCTXIFG:
        ev = UART0_EV_TX;
        break;
    default:
        break;
    }
    uart0_last_event |= ev;

    //sig4_off;

#ifdef LED_SYSTEM_STATES
    sig3_off;
#endif
}
