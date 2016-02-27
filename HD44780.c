#include "HD44780.h"

void hd44780_init(uint8_t bitwidth){ // 4 or 8
    lcd_bus_init();
    // after powerup, wait at least 40ms. _delay_ms is defined for 4MHz, we run at 16MHz.
    _delay_ms(4 * 40);
    lcd_bus_write_cmd4(3);
    // wait > 4.1ms
    _delay_ms(4 * 4.1);
    lcd_bus_write_cmd4(3);
    // wait > 100us
    _delay_us(4 * 100);
    lcd_bus_write_cmd(0x32);// sets 8bit mode, then 4bit mode
    lcd_bus_write_cmd(HD44780_FUNCTION_SET|HD44780_FUNCTION_SET_4BIT|HD44780_FUNCTION_SET_2LINES|HD44780_FUNCTION_SET_5X8FONT);
    lcd_bus_write_cmd(HD44780_DISPLAY|HD44780_DISPLAY_ON);
    lcd_bus_write_cmd(HD44780_CLEAR);
    _delay_ms(4 * 2.1);
    lcd_bus_write_cmd(HD44780_MODE|HD44780_MODE_MOVE_CURSOR|HD44780_MODE_CURSOR_INC);
}

void hd44780_set_cursor(uint8_t line, uint8_t pos){
    switch(line) {
        case 1: lcd_bus_write_cmd(HD44780_SET_DRAM(pos + 64)); break;
        case 2: lcd_bus_write_cmd(HD44780_SET_DRAM(pos + 20)); break;
        case 3: lcd_bus_write_cmd(HD44780_SET_DRAM(pos + 84)); break;
        default: lcd_bus_write_cmd(HD44780_SET_DRAM(pos)); break;
    }
}

void hd44780_put_str(uint8_t* p){
    while (*p) {
        lcd_bus_write_data(*p++);
    }
}
