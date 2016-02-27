#ifndef _4094_H_
#define _4094_H_

#include <inttypes.h>

#define LOW 0
#define HIGH 1

#include "pins.h"


#define PIN_4094_CLK  PIN_D4
#define PIN_4094_DAT  PIN_D0
#define PIN_4094_STB  PIN_D1

void lcd_bus_send4(uint8_t E, uint8_t RS, uint8_t RW, uint8_t data);
void lcd_bus_write_cmd4(uint8_t cmd);
void lcd_bus_init(void);
void lcd_bus_write_cmd(uint8_t cmd);
void lcd_bus_write_data(char data);
uint8_t lcd_bus_read_status(void);
uint8_t lcd_bus_read_data(void);

#endif
