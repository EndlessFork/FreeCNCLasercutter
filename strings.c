

#include "strings.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


#include "lcd.h"

const char PROGMEM nibbles[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

#include <stdarg.h>
// first argument is a funcptr to the output function to use (uart_putchar(), lcd_put_char(), or usb_serial_putchar()
void _P(void (*_f)(char), const char *format, va_list ap);
void _U(void (*_f)(char), uint32_t n);
void _N(void (*_f)(char), int32_t n);
void _F(void (*_f)(char), number f);
void _S(void (*_f)(char), const char *p);
void _B(void (*_f)(char), uint32_t b);

// print a byte in hex
void _XB(void (*_f)(char), uint8_t b) {
    _f(pgm_read_byte(&nibbles[(b >> 4) & 0xf]));
    _f(pgm_read_byte(&nibbles[b & 0xf]));
}

// print a word in hex
void _XW(void (*_f)(char), uint16_t w) {
    _XB(_f, w >> 8);
    _XB(_f, w);
}

// print a dword in hex
void _XD(void (*_f)(char), uint32_t d) {
    _XW(_f, d >> 16);
    _XW(_f, d);
}

// print unsigned int32_t as decimal number
void _U(void (*_f)(char), uint32_t value) {
    uint32_t divider=1;

    if (!value) {
        _f('0');
        return;
    }
    // find highest digit != 0
    divider = 1000000000; // highest power of 10 possible in 32 bits
    while(divider > value) {
        divider /= 10;
    }
    do {
        uint8_t i;
        // now divide:
        for(i='0';value >= divider; i++) {
            value -= divider;
        }
        _f(i);
        divider /= 10;
    } while (divider);
}

// print a 'number' i.e. fixed-point integer with 3 nominals
void _N(void (*_f)(char), int32_t value) {
    int32_t divider=1;

    if (value < 0) {
        _f('-');
        value = -value;
    } else if (!value) {
        _f('0');
        return;
    }
    // find highest digit != 0
    divider = 1000000000; // highest power of 10 possible in 32 bits
    while(divider > value) {
        divider /= 10;
        if (divider == 1000)
            break;
    }
    do {
        uint8_t i;
        // now divide:
        for(i='0';value >= divider; i++) {
            value -= divider;
        }
        _f(i);
        divider /= 10;
        if (divider == 100)
            _f('.');
    } while (divider);
}

// print binary number
void _B(void (*_f)(char), uint32_t b) {
    uint32_t mask = 0x80000000;
    uint8_t i=0;
    _f('0');_f('b');
    for(;mask;mask >>= 1,i++) {
        if (i && ((i&7)==0)) _f('_');
        if (b&mask)
            _f('1');
        else
            _f('0');
    }
}

// main print function
void _P(void (*_f)(char), const char *format, va_list ap) {
    int32_t n;
    uint32_t w;
    char c,*p;
    while ((c = (pgm_read_byte(format++)))) {
        if ('%' == c) {
            switch(c = pgm_read_byte(format++)) {
                case '\0' : format--; // BAD: string ended with '%'
                case 'i':   // signed integer int16_t
                    n = (int16_t) va_arg(ap, int);
                    if (n < 0) {
                        _f('-');
                        _U(_f, (uint32_t) (-n));
                    } else _U(_f, n);
                    break;
                case 'd':    // normal int = uint16_t
                    w = (uint16_t) va_arg(ap, unsigned int);
                    _U(_f, w);
                    break;
                case 'l':    // long integer int32_t
                    n = va_arg(ap, int32_t);
                    if (n < 0) {
                        _f('-');
                        _U(_f, (uint32_t) (-n));
                    } else _U(_f, n);
                    break;
                case 'u':    // unsigned integer uint32_t
                    w = va_arg(ap, uint32_t);
                    _U(_f, w);
                    break;
                case 'f':
                case 'g':
                case 'n': // 'number' fixed point int32_t * BASE
                    n = va_arg(ap, int32_t);
                    _N(_f, n);
                    break;
                case 'b': // binary 32bit blob
                    n = va_arg(ap, int32_t);
                    _B(_f, n);
                    break;
                case 'c': // character
                    c = (char) va_arg(ap, int);
                    _f(c);
                    break;
                case 's': // PROGMEM-string
                    p = (char *) va_arg(ap, void *);
                    while ((c = pgm_read_byte(p++)))
                        _f(c);
                    break;
                case 'S': // RAM-string
                    p = (char *) va_arg(ap, void *);
                    while ((c = *p++))
                        _f(c);
                    break;
                case 'p': // pointer
                    w = (uint32_t) (uint16_t) va_arg(ap, void *);
                    _f('^');
                    _XW(_f, w);
                    break;
                case 'x': // uint16_t as hex
                    w = (uint32_t) va_arg(ap, unsigned int);
                    _f('0');
                    _f('x');
                    _XW(_f, w);
                    break;
                case 'X': // uint32_t as hex
                    w = (uint32_t) va_arg(ap, uint32_t);
                    _f('0');
                    _f('x');
                    _XD(_f, w);
                    break;
                default:
                    _f(c);
            }
        } else
            _f(c);
    }
}

#ifdef DEBUG
void _avrtest_putchar(char c) {avrtest_putchar((int)c);}

void avrtest_print(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    _P(&_avrtest_putchar, fmt, ap);
    va_end(ap);
}
#else
void avrtest_print(const char *fmt, ...) {}

#endif

void cmd_print(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    _P(&cmd_putchar, fmt, ap);
    va_end(ap);
}
void debug_print(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    _P(&debug_putchar, fmt, ap);
    va_end(ap);
}
void lcd_print(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    _P(&lcd_bus_write_data, fmt, ap);
    va_end(ap);
}
