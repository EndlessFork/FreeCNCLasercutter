#ifndef _HD44780_H_
#define _HD44780_H_

#include <inttypes.h>
#include "4094.h"

/* RS R/W
   0   0  CMD
   0   1  read_Status
   1   0  Data_Write
   1   1  Data_Read

   RS & R/W are latched at rising edge of E,
   Data is valid/latched at falling edge of E

  min width of E is

  in 4 bit mode, high nibble is send first
 */

#define HD44780_CLEAR    1
#define HD44780_HOME    2    // + wait 1.52ms
// mode is applied when writing data
#define    HD44780_MODE    4
#define HD44780_MODE_SHIFT_DISPLAY    1
#define HD44780_MODE_MOVE_CURSOR     0
#define HD44780_MODE_CURSOR_INC       2
#define HD44780_MODE_CURSOR_DEC       0
#define HD44780_DISPLAY    8
#define HD44780_DISPLAY_ON    4
#define HD44780_DISPLAY_OFF    0
#define HD44780_DISPLAY_CURSOR_ON    2
#define HD44780_DISPLAY_CURSOR_OFF    0
#define HD44780_DISPLAY_BLINK_ON    1
#define HD44780_DISPLAY_BLINK_OFF    0
// shift manually without writing data
#define HD44780_SHIFT    16
#define HD44780_SHIFT_CURSOR    8
#define HD44780_SHIFT_DISPLAY    0
#define HD44780_SHIFT_RIGHT    4
#define HD44780_SHIFT_LEFT    0
#define HD44780_FUNCTION_SET        32
#define HD44780_FUNCTION_SET_8BIT    16
#define HD44780_FUNCTION_SET_4BIT    0
#define HD44780_FUNCTION_SET_2LINES    8
#define HD44780_FUNCTION_SET_1LINE    0
#define HD44780_FUNCTION_SET_5X10FONT    4
#define HD44780_FUNCTION_SET_5X8FONT    0
#define HD44780_SET_CGRAM(x) ((x)|0x40)
#define HD44780_SET_DRAM(x) ((x)|0x80)
// after each command or data write, wait at least 41us!

void hd44780_init(uint8_t bitwidth);
void hd44780_set_cursor(uint8_t line, uint8_t pos);
void hd44780_put_str(uint8_t* p);
#endif
