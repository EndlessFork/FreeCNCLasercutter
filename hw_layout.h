#ifndef _hw_layout_h_
#define _hw_layout_h_

// load pin definitions
#include "pins.h"

/***********************************
 * Electrical connections          *
 * ----------------------          *
 * These are typically not changed *
 ***********************************/



/****************************************************************************************
* freecnc
****************************************************************************************/
#ifndef __AVR_ATmega32U4__
#error Oops! This only works for ATMega32U4... (on the FreeCNC boards)
#endif

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#ifndef F_TIMER4
// F_CPU, 24, 32, 48, 64MHz
#define F_TIMER4 48000000UL
#endif

/* HW connections are as follows:
    Serial via USB (dedicated pins)
TIMERs PIN    Function
       PF7    TOUCH7     (Touchscreen?, not yet used)
       PF6    TOUCH6     (Touchscreen?, not yet used)
       PF5    TOUCH5     (Touchscreen?, not yet used)
       PF4    TOUCH4     (Touchscreen?, not yet used)
       PF1    Y_HOME     (-> limit sw for Y)
       PF0    X_HOME     (-> limit sw for X)
       PE6    Z_HOME     (not yet connected)
       PE2    HWB        (connected to GND via additional 5K)(Originally open)
T4     PD7    #SD_CS     (SD-card)
T4     PD6    SERVO      (Laser ON/OFF and pwm via #OC4D)
       PD5    TX_LED
       PD4    LCD_CLOCK  (to 4094 CLK)
       PD3    TXD        (UART, for Debugging Output)
       PD2    RXD        (UART, not yet used)
       PD1    LCD_STB    (to 4094 STB)
T0     PD0    LCD_DATA   (to 4094 D)
T4     PC7    Z_STEP     (not yet connected)
T3 T4  PC6    Z_DIR      (not yet connected)
T0 T1  PB7    X_DIR      (to internal DRV8811)
T1 T4  PB6    X_STEP     (to internal DRV8811)
T1 T4  PB5    Y_DIR      (to internal DRV8811)
       PB4    Y_STEP     (to internal DRV8811)
       PB3    MISO       (SD-card, ISP)
       PB2    MOSI       (SD-card, ISP)
       PB1    SCK        (SD-card, ISP)
       PB0    RX_LED

    a 20x4 LCD (HD44780 clone) is connected in 4 bit mode to the 4094 data out lines:
    Q8    LCD_D4
    Q7    LCD_D5
    Q6    LCD_D6
    Q5    LCD_D7
    Q4    LCD_E
    Q3    LCD_R/W
    Q2    LCD_RS
    Q1    not_connected
    for an 8 bit lcd transfer, 6 4094 transfers of 8 bits + stb are needed.

//    Future extension: replace 4094 with '595 and use 8 bit mode
//    (RW->GND, RS->DAT, E->STB, OE->GND) -> faster

    sdcard is connected via buffers (voltage translation to 3.3V)
    X and Y Drivers (DRV8811) are included, Z-driver not.
    Stepping mode is fixed to 8x, current control via poti
    homing switches have pulldowns and connect to +5V if in use (closing)

    F_CPU is going to be 16MHz (5V operation)
    Timer0 ( 8Bit) is for timekeeping (millis() and micros()) (Prescaler=8)
    Timer1 (16Bit) is for the steppers (Prescaler=1) f=244.xx Hz... 20 KHz
    Timer3 (16Bit) is for Laser-Pulse (Prescaler=1..64)
    Timer4 (10+Bit) is for Laser-PWM 11Bit@50Khz
*/


// Define which Pins are used for driving the steppers
// If a pin is not used, just not define it.... do not define with -1 !!!

#define X_STEP_PIN          PIN_B6
#define X_DIR_PIN           PIN_B7
//~ #define X_ENABLE_PIN        none
#define X_MIN_PIN           PIN_F0
//~ #define X_MAX_PIN           none
//~ #define X_HOME_IS_ACTIVE    (READ(X_MIN_PIN))
#define X_HOME_IS_ACTIVE    (READ(X_MIN_PIN)==0)

#define Y_STEP_PIN          PIN_B4
#define Y_DIR_PIN           PIN_B5
//~ #define Y_ENABLE_PIN        none
//~ #define Y_MIN_PIN           none
#define Y_MAX_PIN           PIN_F1
//~ #define Y_HOME_IS_ACTIVE    (READ(Y_MIN_PIN))
#define Y_HOME_IS_ACTIVE    (READ(Y_MAX_PIN)==0)

#define Z_STEP_PIN          PIN_C7
#define Z_DIR_PIN           PIN_C6
//~ #define Z_ENABLE_PIN        none
//~ #define Z_MIN_PIN           PIN_E6
//~ #define Z_MAX_PIN           none
//~ #define Z_HOME_IS_ACTIVE    (READ(Z_MIN_PIN))
//~ #define Z_HOME_IS_ACTIVE    (READ(Z_MIN_PIN)==0)


// For Inverting Stepper Enable Pins (Active Low) use 0, Non Inverting (Active High) use 1
#ifdef X_ENABLE_PIN
    #define ENABLE_X    WRITE(X_ENABLE_PIN, 1)
    #define DISABLE_X   WRITE(X_ENABLE_PIN, 0)
#else
    #define ENABLE_X
    #define DISABLE_X
#endif
#ifdef Y_ENABLE_PIN
    #define ENABLE_Y    WRITE(Y_ENABLE_PIN, 1)
    #define DISABLE_Y   WRITE(Y_ENABLE_PIN, 0)
#else
    #define ENABLE_Y
    #define DISABLE_Y
#endif
#ifdef Z_ENABLE_PIN
    #define ENABLE_Z    WRITE(Z_ENABLE_PIN, 1)
    #define DISABLE_Z   WRITE(Z_ENABLE_PIN, 0)
#else
    #define ENABLE_Z
    #define DISABLE_Z
#endif

// this should always en/-disable all steppers
#define DISABLE_ALL     DISABLE_X;DISABLE_Y;DISABLE_Z
#define ENABLE_ALL      ENABLE_X;ENABLE_Y;ENABLE_Z

// define values of DIR pins for INCrementing or DECrementing coordinates
#define SET_X_INC   WRITE(X_DIR_PIN, 1)
#define SET_X_DEC   WRITE(X_DIR_PIN, 0)
#define SET_Y_INC   WRITE(Y_DIR_PIN, 0)
#define SET_Y_DEC   WRITE(Y_DIR_PIN, 1)
#define SET_Z_INC   WRITE(Z_DIR_PIN, 1)
#define SET_Z_DEC   WRITE(Z_DIR_PIN, 0)

// By default pololu step drivers require an active high signal. However, some high power drivers require an active low signal as step.
// define values of STEP pins for active and inactive
#define SET_X_STEP  WRITE(X_STEP_PIN, 1)
#define CLR_X_STEP  WRITE(X_STEP_PIN, 0)
#define SET_Y_STEP  WRITE(Y_STEP_PIN, 1)
#define CLR_Y_STEP  WRITE(Y_STEP_PIN, 0)
#define SET_Z_STEP  WRITE(Z_STEP_PIN, 1)
#define CLR_Z_STEP  WRITE(Z_STEP_PIN, 0)

// here you may want to activate pullups and stuff
#define INIT_X  SET_OUTPUT(X_STEP_PIN);CLR_X_STEP;SET_OUTPUT(X_DIR_PIN);SET_X_DEC;SET_INPUT(X_MIN_PIN)
#define INIT_Y  SET_OUTPUT(Y_STEP_PIN);CLR_Y_STEP;SET_OUTPUT(Y_DIR_PIN);SET_Y_DEC;SET_INPUT(Y_MAX_PIN)
#define INIT_Z  SET_OUTPUT(Z_STEP_PIN);CLR_Z_STEP;SET_OUTPUT(Z_DIR_PIN);SET_Z_DEC;//SET_INPUT(Z_MIN_PIN)

// define laser control pin(s)
#define LASER_FIRING_PIN PIN_D6 //'SERVO' aka #OC4D
//~ #define LASER_INTENSITY_PIN none


// define LASER_PERIPHERALS (if any)
//~ #define LASER_PERIPHERALS_PIN           -1
//~ #define LASER_PERIPHERALS_STATUS_PIN    -1


// Define Pins for AUX functions
#define SDSS        PIN_D7
#define LED_PIN     PIN_B0

#define TX_LED      PIN_D5
#define RX_LED      PIN_B0

#define UART_TX_PIN PIN_D3
#define UART_RX_PIN PIN_D2

#endif
