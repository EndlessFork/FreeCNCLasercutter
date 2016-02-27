#ifndef _UART_H_
#define _UART_H_

#include <stdint.h>
#include "platform.h"

#include "buffer.h"

#define XON '\x11'
#define XOFF '\x13'

BUFFER_H(UART_RX_BUFFER, uint8_t, UART_RX_BUFFER_SIZE)
BUFFER_H(UART_TX_BUFFER, uint8_t, UART_TX_BUFFER_SIZE)
// These buffers should be powers of two!


#define uart_cantx UART_TX_BUFFER_can_write
#define uart_canrx UART_RX_BUFFER_can_read

void uart_init(uint32_t baud);
void uart_putchar(char c);
char uart_getchar(void);
uint8_t uart_available(void);
#endif
