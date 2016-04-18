


#include "uart.h"
#include "lcd.h"
#include "main.h"

BUFFER(UART_RX_BUFFER, uint8_t, UART_RX_BUFFER_SIZE)
BUFFER(UART_TX_BUFFER, uint8_t, UART_TX_BUFFER_SIZE)

static volatile uint8_t XOFF_STATE;

#define REQUEST_SENDING_XOFF 1
#define XOFF_SEND 2
#define REQUEST_SENDING_XON 4

// Initialize the UART
void uart_init(uint32_t baud) {
    CRITICAL_SECTION_START;
    // always use double speed mode
    UBRR1 = ((((F_CPU) + 4UL * (baud)) / (8UL * (baud)) -1UL));
    UCSR1A = (1<<U2X1);
    UCSR1B = (1<<RXEN1) | (1<<TXEN1) | (1<<RXCIE1);
    UCSR1C = (1<<UCSZ11) | (1<<UCSZ10);
    UART_RX_BUFFER_init();
    UART_TX_BUFFER_init();
    XOFF_STATE = REQUEST_SENDING_XON;
    CRITICAL_SECTION_END;
    uart_putchar(XON);
}

// Transmit a byte
void uart_putchar(char c) {
    if (SIM_ACTIVE) {
        avrtest_putchar(c);
        return;
    }
    while (UART_TX_BUFFER_is_full())
        idle('U');
    CRITICAL_SECTION_START;
    UART_TX_BUFFER_put(c); // is not blocking
    UCSR1B = (1<<RXEN1) | (1<<TXEN1) | (1<<RXCIE1) | (1<<UDRIE1);
    CRITICAL_SECTION_END;
}

char uart_getchar(void) {
    if ((XOFF_STATE & XOFF_SEND) && UART_RX_BUFFER_is_empty()) {
        // initate sending XON
        XOFF_STATE |= REQUEST_SENDING_XON;
        // enabke TX-IRQ
        UCSR1B = (1<<RXEN1) | (1<<TXEN1) | (1<<RXCIE1) | (1<<UDRIE1);
    }
    // eventually wait for new data
    return UART_RX_BUFFER_get();
}

uint8_t uart_available(void) {
    CRITICAL_SECTION_START;
    if ((XOFF_STATE & XOFF_SEND) && UART_RX_BUFFER_is_empty()) {
        // initate sending XON
        XOFF_STATE |= REQUEST_SENDING_XON;
        // enabke TX-IRQ
        UCSR1B = (1<<RXEN1) | (1<<TXEN1) | (1<<RXCIE1) | (1<<UDRIE1);
    }
    CRITICAL_SECTION_END;
    return UART_RX_BUFFER_used();
}


// Transmit Interrupt
ISR(USART1_UDRE_vect)
{
    ACTIVE_IRQ_7;
    CHECK_STACK;
    // XON/XOFF handling (hold TX buffer)
    if (XOFF_STATE & REQUEST_SENDING_XOFF) {
        UDR1 = XOFF;
        XOFF_STATE = XOFF_SEND;
    } else if (XOFF_STATE & REQUEST_SENDING_XON) {
        UDR1 = XOFF;
        XOFF_STATE = 0;
    } else if (UART_TX_BUFFER_is_empty()) {
        // buffer is empty, disable transmit interrupt
        UCSR1B = (1<<RXEN1) | (1<<TXEN1) | (1<<RXCIE1);
    } else {
        UDR1 = UART_TX_BUFFER_data[UART_TX_BUFFER_tail];
        UART_TX_BUFFER_pop();
    }
    ACTIVE_IRQ_NONE;
}

extern void kill(void);
// Receive Interrupt
ISR(USART1_RX_vect)
{
    //~ uint8_t c;
//~
    //~ c = UDR1;
    // quite full -> send XOFF
    ACTIVE_IRQ_3;
    CHECK_STACK;
    if (UART_RX_BUFFER_used() > UART_RX_BUFFER_SIZE/4) {
        // if no XOFF send already, initiate sending XOFF
        if (~XOFF_STATE & XOFF_SEND) {
            // request sending XOFF
            XOFF_STATE |= REQUEST_SENDING_XOFF;
            // enable TX-IRQ
            UCSR1B = (1<<RXEN1) | (1<<TXEN1) | (1<<RXCIE1) | (1<<UDRIE1);
        }
    }

    // if fifo is full, kill
    if (!UART_RX_BUFFER_can_write()) {
        lcd_print_at(0,3,PSTR("UART_RX_FULL")); kill();
        //~ UART_RX_BUFFER_pop(); //  remove oldest char
    }
    UART_RX_BUFFER_put(UDR1);
    ACTIVE_IRQ_NONE;
}
