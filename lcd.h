#ifndef _LCD_H_
#define _LCD_H_

#include "4094.h"
#include "HD44780.h"
#include "pins.h"
#include "platform.h"
#include "time.h"
#include "stepper.h"
#include "strings.h"
#include "planner.h"

#ifdef DEBUG
#define lcd_print_at(y,x,...) {LOG_STRING("LCD:");LOG_U8(y);LOG_U8(x);avrtest_print(__VA_ARGS__);LOG_NEWLINE;}
#else
#define lcd_print_at(y,x,...) {lcd_set_cursor(y,x);lcd_print(__VA_ARGS__);}
#endif

void lcd_init(void);
void lcd_update(uint8_t c);
#define lcd_set_cursor(y,x) hd44780_set_cursor(y,x)
#define lcd_put_str(p)    hd44780_put_str(p)
#define lcd_put_char(c) lcd_bus_write_data(c)

#endif
