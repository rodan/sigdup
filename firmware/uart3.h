#ifndef __UART3_H__
#define __UART3_H__

#ifdef __cplusplus
extern "C" {
#endif

#define    UART3_EV_NULL  0
#define      UART3_EV_RX  0x1
#define      UART3_EV_TX  0x2

#define    BAUDRATE_9600  0x1
#define   BAUDRATE_19200  0x2
#define   BAUDRATE_57600  0x3

#define   UART3_RXBUF_SZ  128

void uart3_init(const uint8_t speed);
void uart3_port_init(void);
void uart3_port_deinit(void);
uint16_t uart3_tx_str(const char *str, const uint16_t size);
uint16_t uart3_print(const char *str);
uint8_t uart3_get_event(void);
void uart3_rst_event(void);
void uart3_set_eol(void);
void uart3_disable_rx(void);
char *uart3_get_rx_buf(void);
char *uart3_get_rx_buf_force(void);
uint8_t uart3_get_rx_buf_len(void);
void uart3_set_intrchar_tmout(const uint16_t tmout);

#ifdef __cplusplus
}
#endif

#endif
