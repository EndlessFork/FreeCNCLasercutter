#ifndef _time_h_
#define _time_h_

#include <stdint.h>
#include "platform.h"
#include "pins.h"
#include "hw_layout.h"

void time_init(void);

uint32_t millis(void);
uint32_t micros(void);

void delay_ms(uint32_t);
void delay_us(uint32_t);

extern volatile uint8_t time_flags;

#define TIME_FLAG_1MS    1
#define TIME_FLAG_10MS    2
#define TIME_FLAG_100MS    4
#define TIME_FLAG_1S    8
#define TIME_FLAG_10S    16
#define TIME_FLAG_1MIN    32
#define TIME_FLAG_10MIN    64
#define TIME_FLAG_1H    128


#endif
