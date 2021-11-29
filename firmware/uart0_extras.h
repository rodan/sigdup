#ifndef __UART0_EXTRAS_H__
#define __UART0_EXTRAS_H__

#define intrchar_tmout 500      // _2ms
#define  UART_RET_WAKE 1
#define UART_RET_SLEEP 2

#ifdef __cplusplus
extern "C" {
#endif



    typedef enum {
        RX_USER,
        RX_ZMODEM_HDR,
        RX_ZMODEM_DATA
    } input_type;

    uint8_t uart0_get_input_type(void);
    void uart0_set_input_type(const uint8_t type);
    uint8_t uart0_extra_irq_handler(const uint8_t c);

#ifdef __cplusplus
}
#endif
#endif
