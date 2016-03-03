
#include "main.h"
#include "parser.h"
#include "lcd.h"
#include "stepper.h"
#include "planner.h"
#include "time.h"
#include "strings.h"

#include "laser.h"
#include "watchdog.h"

#ifdef DEBUG
uint8_t SIM_ACTIVE;
#endif

void send_str(const uint8_t *s);
uint8_t recv_str(uint8_t *buf, uint8_t size);
void parse_and_execute_command(const uint8_t *buf, uint8_t num);

//Inactivity shutdown variables
//~ static uint32_t previous_millis_cmd = 0;
//~ static uint32_t max_inactive_time = 0;
//~ static uint32_t stepper_inactive_time = 5*1000l;

uint32_t starttime=0;
uint32_t stoptime=0;

uint16_t MIN_SP = RAMEND; // stores lowest value of stack pointer.....

bool Stopped=FALSE;


void setup(void){
    // set for full-speed (16MHz clock)

    CLKPR = 0x80;    // enable clocksetting bit
    CLKPR = 0x00;    // set divider to 1
    SET_OUTPUT(TX_LED);
    SET_OUTPUT(RX_LED);
    SET_PIN(TX_LED);
    SET_PIN(RX_LED);

    INIT_ALL;    // init pins
    time_init();    // init timer0 and timekeeping: micros, millis
    if (!SIM_ACTIVE)
        usb_init();
    uart_init(BAUDRATE);
    //~ usb_serial_flush_input();
    //~ usb_serial_flush_output();
    laser_init();
    if (!SIM_ACTIVE)
        lcd_init();
    planner_init();
    stepper_init();
    watchdog_init();
    #ifdef USE_WATCHDOG
    watchdog_init();
    #endif

    // loads data from EEPROM if available else uses defaults (and resets step Config.accel rate)
    //~ Config_RetrieveSettings();
    sei();

    // Check startup - does nothing if bootloader sets MCUSR to 0
    //~ uint8_t mcu = MCUSR;
    //~ if(mcu & 1) SERIAL_ECHOLNPGM(MSG_POWERUP);
    //~ if(mcu & 2) SERIAL_ECHOLNPGM(MSG_EXTERNAL_RESET);
    //~ if(mcu & 4) SERIAL_ECHOLNPGM(MSG_BROWNOUT_RESET);
    //~ if(mcu & 8) SERIAL_ECHOLNPGM(MSG_WATCHDOG_RESET);
    //~ if(mcu & 32) SERIAL_ECHOLNPGM(MSG_SOFTWARE_RESET);
    //~ MCUSR=0;

    parser_init(); // execute inital G28
}

void manage_inactivity(void);

void idle(char c){
    CHECK_STACK;
#ifdef DEBUG
    LOG_STRING("idle:");LOG_CHAR(c);LOG_NEWLINE;
    if (SIM_ACTIVE) {
        if (!UART_TX_BUFFER_is_empty())
            USART1_UDRE_vect();
        LOG_STRING("M: after LOOP, calling IRQ's\n");
        // time keeping
        TIMER0_COMPA_vect();
        // laser pulsing (if enabled)
        if (TCCR3B & 7)
            TIMER3_COMPA_vect();
        // stepper Stepping
        //~ LOG_ON;
        PERF_PLABEL(1,"M: Stepper IRQ timing");
        PERF_START(1);
        TIMER1_COMPA_vect();
        PERF_STOP(1);
        //~ LOG_OFF;
    };
#endif
    watchdog_reset();
    if (time_flags & TIME_FLAG_1S) {
        time_flags &=~TIME_FLAG_1S;
        manage_inactivity();
    }
    if (time_flags & TIME_FLAG_100MS) {
        time_flags &=~ TIME_FLAG_100MS;
        TOGGLE(TX_LED);
        lcd_update(c);
    }
}

void loop(void) {
    char c;
    watchdog_reset();
    if (cmd_available()) {
        c = cmd_getchar();
        // process_char calls process_command, once a command is complete...

        CLR_PIN(RX_LED); // enable LED (active LOW)
        switch (process_char(c)) {
            PARSER_OK :
                // reply OK and free entries in stepper queue
                cmd_print(PSTR("OK %d\n"), STEPPER_QUEUE_SIZE - 1 - STEPPER_QUEUE_used());
                cmd_flush();
            PARSER_CHECKSUM_ERROR :
                // reply with resend request
                cmd_print(PSTR("resend!\n"));
                cmd_flush();
            PARSER_FORMAT_ERROR :
                // reply with error:
                cmd_print(PSTR("FORMAT ERROR\n"));
                cmd_flush();
            PARSER_NEXTCHAR :
                break;
        }
        SET_PIN(RX_LED); // disable LED
    }
#ifdef DEBUG
    if (SIM_ACTIVE) {
        //~ LOG_ON;
        if (avrtest_chars_avail())
            process_char(avrtest_getchar());
        //~ LOG_OFF;
    }
#endif
    idle('L');
}

int main(void){
    // check if avrtest is running
    // XXX: use avrtest_chars_avail (SYSCALL 26), code may be smaller.
#ifdef DEBUG
    avrtest_print("KKK ->%x<-\n",MIN_SP);
    LOG_OFF;
    asm volatile (
    "ldi   r20, 0xff  \n\t"
    "clr   r24        \n\t"
    "ldi   r25, 0xF0        \n\t"
    "cpse r27,r27       \n\t"
    ".word 0xffff \n\t"
    "com   r20    \n\t"
    "asr   r20    \n\t"
    "mov   %A0, r20 \n\t"
    : \
    "=&r" ((uint8_t) SIM_ACTIVE) \
    :: "r20", "r21", "r22", "r23", "r24", "r25");
#endif
    setup();
    while (TRUE) {
        loop();
    }
}


void manage_inactivity(void) {
    //~ if ((millis() - previous_millis_cmd) > max_inactive_time) {
        //~ if (stepper_inactive_time) {
            //~ if ((millis() - previous_millis_cmd) > stepper_inactive_time)    {
                //~ if(STEPPER_QUEUE_is_empty()) {
                    //~ DISABLE_ALL;
                    //~ laser_init();
                    //~ #ifdef LASER_PERIPHERALS
                    //~ laser_peripherals_off();
                    //~ #endif
                //~ }
            //~ }
        //~ }
    //~ }
    if (laser.lasing_time)
        laser_add_time(0);
}

#define reset _VECTOR(0)
void kill(void)
{
    lcd_print_at(2,0, "KILL !!!!");

    cli(); // Stop interrupts

    laser_fire(0);
    laser_set_mode(LASER_OFF);

    DISABLE_ALL;

    #ifdef LASER_PERIPHERALS
    laser_peripherals_off();
    #endif // LASER_PERIPHERALS

    _delay_ms(5000);  // wait a little, then reboot
    //~ reset();
    asm( "JMP 0 \n\t");
}
