#ifndef _PLATFORM_H_
#define _PLATFORM_H_

//~ #define DEBUG

// include git version as VERSION define
#include "Version"

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>


#include <avr/io.h>
#include <avr/common.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

extern uint16_t MIN_SP;
#define CHECK_STACK {if (SP < MIN_SP) MIN_SP=SP;}

#ifdef DEBUG
#include "avrtest/avrtest.h"
#define LOG_NEWLINE LOG_CHAR('\n')
#define LOG_SPACE   LOG_CHAR(' ')
#define LOG_COMMA   LOG_CHAR(',')
#define LOG_STRING(x) {LOG_PSTR(PSTR(x));}
#else
#define LOG_NEWLINE {}
#define LOG_SPACE   {}
#define LOG_COMMA   {}
#define LOG_STRING(x) {}
#define LOG_U32(x)  {}
#define LOG_S32(x)  {}
#define LOG_X32(x)  {}
#define LOG_U24(x)  {}
#define LOG_S24(x)  {}
#define LOG_X24(x)  {}
#define LOG_U16(x)  {}
#define LOG_S16(x)  {}
#define LOG_X16(x)  {}
#define LOG_U8(x)  {}
#define LOG_S8(x)  {}
#define LOG_X8(x)  {}
#define LOG_CHAR(x) {}
#define avrtest_chars_avail() 0
#define avrtest_getchar() 0
#define avrtest_abort() {}
#define avrtest_putchar(x) {}
#define PERF_START(x) {}
#define PERF_STOP(x) {}
#define PERF_PLABEL(x,y) {}
#define PERF_DUMP_ALL {}
#define LOG_ON {}
#define LOG_OFF {}
#endif


#include "hw_config.h"


#ifdef DEBUG
extern uint8_t SIM_ACTIVE;
#else
#define SIM_ACTIVE 0
#endif

#ifdef __INT24_MAX__
typedef __int24 int24_t;
typedef __uint24 uint24_t;
#else
#warning "no Int24! falling back to int32"
typedef int32_t int24_t;
typedef uint32_t uint24_t;
#endif

#define  FORCE_INLINE __attribute__((always_inline)) inline

#ifndef CRITICAL_SECTION_START
  #define CRITICAL_SECTION_START  uint8_t _sreg = SREG; cli();
  #define CRITICAL_SECTION_END    SREG = _sreg;
#endif //CRITICAL_SECTION_START


#ifndef bool
    #define bool uint8_t
#endif
#ifndef TRUE
    #define TRUE    ((bool) 1)
    #define FALSE    ((bool) 0)
#endif

#ifndef NULL
    #define NULL ((void*) 0)
#endif

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define sgn(x) ((x < 0) ? -1 : (x > 0))

// defines how many 'sub-digits' are stored
// 3 means: 0.123456789... is stored as 123
#define SCALE_DIGITS 3
// this is the value of '1' in scaled units
// 3 means: 1000
#define ONE 1000
// one scaled_int unit is worth that much in number
#define BASE 1/1000
// this should be True: ONE*BASE == 1

// make it more clear in the code when we have this scaled int's
typedef int32_t number;
// -8mill to 8mill / 1000 -> ~-8000...8000 units
#define MAX_NUMBER 8388607
#define MIN_NUMBER -8388608

typedef uint8_t mask_t;
// en/disable parts of movement in axis_mask
#define X_AXIS_MASK ((uint8_t) 0b0001)
#define Y_AXIS_MASK ((uint8_t) 0b0010)
#define Z_AXIS_MASK ((uint8_t) 0b0100)
#define LASER_MASK  ((uint8_t) 0b1000)

typedef int24_t StepperPos;
typedef uint24_t StepperCnt;

#include "buffer.h"

// evaluate serial setting
#ifdef SERIAL_PORT_UART
    #ifdef SERIAL_PORT_USB
        #error "please use only one of SERIAL_PORT_USB or SERIAL_PORT_UART!"
    #endif
    #include "uart.h"
    #if UART_TX_BUFFER_SIZE < 8
        #error "UART_TX_BUFFER_SIZE needs to be at least 8"
    #endif
    #if UART_RX_BUFFER_SIZE < 64
        #error "UART_RX_BUFFER_SIZE needs to be at least 64"
    #endif
    #define cmd_getchar uart_getchar
    #define cmd_available uart_available
    #define cmd_putchar uart_putchar
    #define cmd_flush() {} /* XXX: ???? */
    // the normally not used port is the debug port....
    #include "usb_serial.h"
    #define debug_putchar usb_serial_putchar
    #define debug_flush usb_serial_flush_output
#else
    #ifdef SERIAL_PORT_USB
        #include "usb_serial.h"
        #define cmd_getchar usb_serial_getchar
        #define cmd_available() (usb_configured() && usb_serial_available())
        #define cmd_putchar usb_serial_putchar_nowait
        #define cmd_flush usb_serial_flush_output
        // the normally not used port is the debug port....
        #include "uart.h"
        #if UART_TX_BUFFER_SIZE < 64
            #error "UART_TX_BUFFER_SIZE needs to be at least 64"
        #endif
        #define debug_putchar uart_putchar
        #define debug_flush() {} /* XXX: ???? */
    #else
        #error "please use exactly one of SERIAL_PORT_USB or SERIAL_PORT_UART!"
    #endif
#endif

#include "strings.h"

#endif
