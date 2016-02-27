#include "4094.h"

// Originalschaltung
void lcd_bus_send4(uint8_t E, uint8_t RS, uint8_t RW, uint8_t data){
    // data is MSDB=-, rs, rw, e, d7, d6, d5, d4=LSB
    // shift out in this order: d4, d5, d6, d7, e, rw, rs, -
    uint8_t i;
    WRITE(PIN_4094_STB, LOW);
    data = data & 0x0F;
    if (RS) data |= 0x40;
    if (RW) data |= 0x20;
    if (E) data |= 0x10;
    for(i=0;i<8;i++) {
        WRITE(PIN_4094_CLK, LOW);
        WRITE(PIN_4094_DAT, data & 1);
        WRITE(PIN_4094_CLK, HIGH);
        data = data >> 1;
    }
    WRITE(PIN_4094_STB, HIGH);
    WRITE(PIN_4094_CLK, LOW);
    WRITE(PIN_4094_STB, LOW);
}

void lcd_bus_write_cmd4(uint8_t cmd){
    // Write-> RW=0, CMD -> RS=0
    cmd &= 0x0f;
    //            E  RS RW dat4
    lcd_bus_send4(0, 0, 0, cmd);
    lcd_bus_send4(1, 0, 0, cmd);
    lcd_bus_send4(0, 0, 0, cmd);
} 

void lcd_bus_init(void){
    CLR_PIN(PIN_4094_STB);
    SET_OUTPUT(PIN_4094_STB);
    CLR_PIN(PIN_4094_DAT);
    SET_OUTPUT(PIN_4094_DAT);
    CLR_PIN(PIN_4094_CLK);
    SET_OUTPUT(PIN_4094_CLK);
    lcd_bus_send4(0, 0, 0, 0);
}


void lcd_bus_write_cmd(uint8_t cmd){
    lcd_bus_write_cmd4(cmd >> 4);
    lcd_bus_write_cmd4(cmd);
    while (lcd_bus_read_status() & 0x80) ;
}

void lcd_bus_write_data(char data){
    uint8_t dat4=data >> 4;
    // Write-> RW=0, DATA -> RS=1
    //            E  RS RW dat4
    lcd_bus_send4(0, 1, 0, dat4);
    lcd_bus_send4(1, 1, 0, dat4);
    lcd_bus_send4(0, 1, 0, dat4);
    dat4 = data & 0x0f;
    //            E  RS RW dat4
    lcd_bus_send4(0, 1, 0, dat4);
    lcd_bus_send4(1, 1, 0, dat4);
    lcd_bus_send4(0, 1, 0, dat4);
    while (lcd_bus_read_status() & 0x80) ;
}

uint8_t lcd_bus_read_status(void){
    _delay_us(50);
    return 0;
}
uint8_t lcd_bus_read_data(void){
    return 0;
}
