
#include "lcd.h"
#include "coordinates.h"

#include "stepper.h"
// for accessing stepper state:
extern state_t STATE;

//~ // init lcd and show splash-screen
//~ const uint8_t custom_font[] PROGMEM = {
    //~ // char 0
    //~ 0b00000,
    //~ 0b00000,
    //~ 0b00000,
    //~ 0b00000,
    //~ 0b00000,
    //~ 0b00001,
    //~ 0b00001,
    //~ 0b00010,
    //~ // char 1
    //~ 0b00100,
    //~ 0b01010,
    //~ 0b10001,
    //~ 0b10001,
    //~ 0b00000,
    //~ 0b00000,
    //~ 0b00000,
    //~ 0b00100,
    //~ // char 2
    //~ 0b00000,
    //~ 0b00000,
    //~ 0b00000,
    //~ 0b00000,
    //~ 0b00000,
    //~ 0b10000,
    //~ 0b10000,
    //~ 0b01000,
    //~ // char 3
    //~ 0b00100,
    //~ 0b00100,
    //~ 0b01001,
    //~ 0b01000,
    //~ 0b10000,
    //~ 0b10000,
    //~ 0b10000,
    //~ 0b01111,
    //~ // char 4
    //~ 0b10101,
    //~ 0b01110,
    //~ 0b11111,
    //~ 0b01110,
    //~ 0b10101,
    //~ 0b00100,
    //~ 0b00000,
    //~ 0b11111,
    //~ // char 5
    //~ 0b00100,
    //~ 0b00100,
    //~ 0b11010,
    //~ 0b00010,
    //~ 0b00001,
    //~ 0b00001,
    //~ 0b00001,
    //~ 0b11110,
    //~ // char 6
    //~ 0b00100,
    //~ 0b11111,
    //~ 0b10001,
    //~ 0b01010,
    //~ 0b01010,
    //~ 0b00100,
    //~ 0b11111,
    //~ 0b00100,
    //~ // char 7
    //~ 0b01110,
    //~ 0b00110,
    //~ 0b01000,
    //~ 0b10110,
    //~ 0b00110,
    //~ 0b01010,
    //~ 0b10000,
    //~ 0b00000
//~ };

void lcd_clear(void){
    lcd_bus_write_cmd(HD44780_CLEAR);
    _delay_ms(7); // >4*1.53 ms
}

void lcd_init(void){
    _delay_ms(4*50);
    hd44780_init(4);
    hd44780_init(4); //delays too short? one call works 50% of the time.
    // load custom chars
    //~ lcd_bus_write_cmd(HD44780_SET_CGRAM(0)); // start load at first cgram addr
    //~ for(uint8_t i=0; i<sizeof(custom_font);i++) {
        //~ lcd_bus_write_data(pgm_read_byte(&custom_font[i]));
    //~ }
    lcd_clear();
    lcd_update('I');
}




void lcd_update(uint8_t c){
    int32_t tmp;
    //~ lcd_clear();
    //~ lcd_set_cursor(0,0);
    //~ lcd_put_char(0);
    //~ lcd_put_char(1);
    //~ lcd_put_char(2);
    //~ hd44780_put_str(" Lasercutter    ");
    //~ lcd_put_char(c);
    //~ lcd_set_cursor(1,0);
    //~ lcd_put_char(3);
    //~ lcd_put_char(4);
    //~ lcd_put_char(5);
    //~ hd44780_put_str(" by Enno : ");
    //~ uint8_t ic=' ';
    //~ for(uint32_t t=millis(), d=10000000;d;d/=10) {
        //~ uint8_t c = ic;
        //~ if ((t>d) || (d==1)) {
            //~ c = ic = '0';
        //~ }
        //~ while (t>d) {
            //~ c++;
            //~ t-=d;
        //~ }
        //~ lcd_put_char(c);
    //~ }
//~
    //~ lcd_set_cursor(2,0);
    //~ hd44780_put_str("\6\7  build: ");
    //~ for(uint8_t t=TCNT0, d=100;d;d/=10) {
        //~ uint8_t c='0';
        //~ while (t>d) {
            //~ c++;
            //~ t-=d;
        //~ }
        //~ lcd_put_char(c);
    //~ }
    //~ lcd_set_cursor(3,0);
    //~ hd44780_put_str(__DATE__ " " __TIME__);
    //~ uint8_t ctr;

    lcd_print_at(0,0,PSTR("%c Queue:%d %n "), c, STEPPER_QUEUE_used(), millis());

    tmp =  X_steps2position(stepper_get_position(0));
    lcd_print_at(1,0,PSTR("X:%n            "),tmp);
    tmp =  Y_steps2position(stepper_get_position(1));
    lcd_print_at(2,0,PSTR("Y:%n            "),tmp);
    tmp =  Z_steps2position(stepper_get_position(2));
    lcd_print_at(3,0,PSTR("Z:%n            "),tmp);
    if (STATE.laser.mode)
        lcd_print_at(1,10,PSTR("LASER ON %d"), STATE.laser.mode)
    else
        lcd_print_at(1,10,PSTR("laser OFF"));
    lcd_print_at(2,10,PSTR("%d %d %d "), STATE.laser.mode, STATE.laser.modulation+1, STATE.laser.power);
#ifdef DEBUG
    lcd_print_at(3,10,PSTR("STACK: %x"), MIN_SP);
    LOG_STRING("MIN_SP");LOG_X16(MIN_SP);LOG_NEWLINE;
#else
    CHECK_STACK;
    lcd_print_at(3,10,PSTR("SP:%x"), MIN_SP);
    //~ lcd_print_at(3,11,PSTR("%n us"), STATE.laser.pulse_duration_us);
    //~ uart_print(PSTR("Stack: %x\n"), MIN_SP);
#endif
}

