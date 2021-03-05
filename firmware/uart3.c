
#include <msp430.h>
#include <inttypes.h>
#include <string.h>
#include "uart3.h"
#include "timer_a0.h"

// get the UART3_SPEED_ #define
#include "config.h"

volatile char uart3_rx_buf[UART3_RXBUF_SZ];     // receive buffer
volatile uint8_t uart3_p;       // number of characters received, 0 if none
volatile uint8_t uart3_rx_enable;
volatile uint8_t uart3_last_event;
volatile uint16_t uart3_intrchar_tmout;

// you'll have to initialize/map uart ports in main()
// or use uart3_port_init() if no mapping is needed
void uart3_init(const uint8_t baudrate)
{
    UCA3CTLW0 = UCSWRST;        // put eUSCI state machine in reset

    uart3_port_init();

    // consult 'Recommended Settings for Typical Crystals and Baud Rates' in slau367o
    // for some reason any baud >= 115200 ends up with a non-working RX channel
    // UCA3BRW = UCBRx
    // UCA3MCTLW = UCOS16 + UCBRFx << 4 + UCBRSx << 8

    switch (baudrate) {
        case BAUDRATE_9600:
            UCA3CTLW0 |= UCSSEL__SMCLK;
            UCA3BRW = 52;
            UCA3MCTLW = 0x4911;
            break;
        case BAUDRATE_19200:
            UCA3CTLW0 |= UCSSEL__SMCLK;
            UCA3BRW = 26;
            UCA3MCTLW = 0xb601;
            uart3_intrchar_tmout = 500; // _2ms
            break;
        case BAUDRATE_57600:
            // UCOS16  UCBRx   UCBRFx  UCBRSx
            // 1       8       10      0xF7
            UCA3CTLW0 |= UCSSEL__SMCLK;
            UCA3BRW = 8;
            UCA3MCTLW = 0xf7a1;
            //intrchar_tmout = 166;
            uart3_intrchar_tmout = 500;
            break;
    }
          
    /*
    break;
    case UART3_SPEED_19200_1M:
#elif defined (UART3_SPEED_38400_1M)
    UCA3CTLW0 |= UCSSEL__SMCLK;
    UCA3BRW = 1;
    UCA3MCTLW = 0x00a1;
#elif defined (UART3_SPEED_115200_1M)
    UCA3CTLW0 |= UCSSEL__SMCLK;
    UCA3BRW = 8;
    UCA3MCTLW = 0xd600;
#elif defined (UART3_SPEED_38400_8M)
    UCA3CTLW0 |= UCSSEL__SMCLK;
    UCA3BRW = 13;
    UCA3MCTLW = 0x8401;
#elif defined (UART3_SPEED_115200_8M)
    UCA3CTLW0 |= UCSSEL__SMCLK;
    UCA3BRW = 4;
    UCA3MCTLW = 0x5551;
#elif defined (UART3_SPEED_230400_8M)
    UCA3CTLW0 |= UCSSEL__SMCLK;
    UCA3BRW = 2;
    UCA3MCTLW = 0xbb21;
#elif defined (UART3_SPEED_460800_8M)
    UCA3CTLW0 |= UCSSEL__SMCLK;
    UCA3BRW = 17;
    UCA3MCTLW = 0x4a00;
#else // a safer default of 9600 - does not depend on SMCLK
    UCA3CTLW0 |= UCSSEL__ACLK;
    UCA3BRW = 3;
    //UCA3MCTLW |= 0x5300;
    UCA3MCTLW |= 0x9200;
#endif
*/
    UCA3CTLW0 &= ~UCSWRST;      // Initialize eUSCI
    //UCA3IE |= UCRXIE;           // Enable USCI_A3 RX interrupt

    uart3_p = 0;
    uart3_rx_enable = 1;
}

// default port locations
void uart3_port_init(void)
{
    P6SEL0 |= (BIT0 | BIT1);
    P6SEL1 &= ~(BIT0 | BIT1);
}

void uart3_port_deinit(void)
{
    P6SEL0 &= ~(BIT0 | BIT1);
    P6SEL1 &= ~(BIT0 | BIT1);
}

uint8_t uart3_get_event(void)
{
    return uart3_last_event;
}

void uart3_rst_event(void)
{
    uart3_last_event = UART3_EV_NULL;
}

void uart3_set_eol(void)
{
    uart3_p = 0;
    uart3_rx_enable = 1;
}

void uart3_set_intrchar_tmout(const uint16_t tmout)
{
    uart3_intrchar_tmout = tmout;
}

void uart3_disable_rx(void)
{
    uart3_rx_enable = 0;
}

char *uart3_get_rx_buf(void)
{
    if (uart3_p) {
        return (char *)uart3_rx_buf;
    } else {
        return NULL;
    }
}

char *uart3_get_rx_buf_force(void)
{
    return (char *)uart3_rx_buf;
}

uint8_t uart3_get_rx_buf_len(void)
{
    return uart3_p;
}

uint16_t uart3_tx_str(const char *str, const uint16_t size)
{
    uint16_t p = 0;
    while (p < size) {
        while (!(UCA3IFG & UCTXIFG)) {
        }                       // USCI_A3 TX buffer ready?
        UCA3TXBUF = str[p];
        p++;
    }
    while (!(UCA3IFG & UCTXIFG)) {}
    return p;
}

uint16_t uart3_print(const char *str)
{
    size_t p = 0;
    size_t size = strlen(str);
    while (p < size) {
        while (!(UCA3IFG & UCTXIFG)) {
        }                       // USCI_A3 TX buffer ready?
        UCA3TXBUF = str[p];
        p++;
    }
    while (!(UCA3IFG & UCTXIFG)) {}
    return p;
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=EUSCI_A3_VECTOR
__interrupt void USCI_A3_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(EUSCI_A3_VECTOR))) USCI_A3_ISR(void)
#else
#error Compiler not supported!
#endif
{
    uint16_t iv = UCA3IV;
    register char rx;
    uint8_t ev = 0;
            
    switch (iv) {
    case USCI_UART_UCRXIFG:
#ifdef LED_SYSTEM_STATES
        sig3_on;
#endif
        rx = UCA3RXBUF;
        if (uart3_rx_enable && (uart3_p < UART3_RXBUF_SZ - 1)) {
            uart3_rx_buf[uart3_p] = rx;
            uart3_rx_buf[uart3_p+1] = 0;
            uart3_p++;
            ev = UART3_EV_RX;
            timer_a0_delay_noblk_ccr1(uart3_intrchar_tmout);
        }
        break;
    case USCI_UART_UCTXIFG:
        ev = UART3_EV_TX;
        break;
    default:
        break;
    }
    uart3_last_event |= ev;
#ifdef LED_SYSTEM_STATES
    sig3_off;
#endif
}
