#ifndef _PINS_H_
#define _PINS_H_

#include <avr/io.h>
#include <util/delay.h>

// +0 is PINx, +1 is DDRx, +2 is PORTx
#define _SET_PIN(_pin)        do {_SFR_IO8(2+3*(_pin >> 3)) |=  (1<<(_pin & 7));} while (0)
#define _CLR_PIN(_pin)        do {_SFR_IO8(2+3*(_pin >> 3)) &=~ (1<<(_pin & 7));} while (0)
#define _TOGGLE_PIN(_pin)    do {_SFR_IO8(0+3*(_pin >> 3)) |=  (1<<(_pin & 7));} while (0)
#define _SET_OUTPUT(_pin)    do {_SFR_IO8(1+3*(_pin >> 3)) |=  (1<<(_pin & 7));} while (0)
#define _SET_PULLUP(_pin)    do {_SFR_IO8(1+3*(_pin >> 3)) &=~  (1<<(_pin & 7));_SFR_IO8(2+3*(_pin >> 3)) |=  (1<<(_pin & 7));} while (0)
#define _SET_INPUT(_pin)    do {_SFR_IO8(1+3*(_pin >> 3)) &=~  (1<<(_pin & 7));_SFR_IO8(2+3*(_pin >> 3)) &=~ (1<<(_pin & 7));} while (0)
#define _WRITE(_pin,value)    do {if(value)_SET_PIN(_pin);else _CLR_PIN(_pin);} while(0)
#define _READ(_pin)            ((bool) (_SFR_IO8(0+3*(_pin >> 3)) & (1<<(_pin & 7))))
#define _GET_INPUT(_pin)    ((_SFR_IO8(1+3*(_pin >> 3)) & (1<<(_pin & 7));)==0)
#define _GET_OUTPUT(_pin)    ((_SFR_IO8(1+3*(_pin >> 3)) & (1<<(_pin & 7));)!=0)

//set/clr/toggle a pin
#define SET_PIN(pindef)        _SET_PIN(pindef)
#define CLR_PIN(pindef)        _CLR_PIN(pindef)
#define TOGGLE(pindef)        _TOGGLE_PIN(pindef)
// set in/put mode
#define SET_OUTPUT(pindef)    _SET_OUTPUT(pindef)
#define SET_PULLUP(pindef)    _SET_PULLUP(pindef)
#define SET_INPUT(pindef)    _SET_INPUT(pindef)
// write a value to a pin
#define WRITE(pindef,value)    _WRITE(pindef,(value))
// read input value
#define READ(pindef)        _READ(pindef)
// read mode
#define GET_INPUT(pindef)    _GET_INPUT(pindef)
#define GET_OUTPUT(pindef)    _GET_OUTPUT(pindef)

#define GET_TIMER(x)    x ## _TIMER


#define PIN_A0    0
#define PIN_A1    1
#define PIN_A2    2
#define PIN_A3    3
#define PIN_A4    4
#define PIN_A5    5
#define PIN_A6    6
#define PIN_A7    7

#define PIN_B0    8
#define PIN_B1    9
#define PIN_B2    10
#define PIN_B3    11
#define PIN_B4    12
#define PIN_B5    13
#define PIN_B6    14
#define PIN_B7    15

#define PIN_C0    16
#define PIN_C1    17
#define PIN_C2    18
#define PIN_C3    19
#define PIN_C4    20
#define PIN_C5    21
#define PIN_C6    22
#define PIN_C7    23

#define PIN_D0    24
#define PIN_D1    25
#define PIN_D2    26
#define PIN_D3    27
#define PIN_D4    28
#define PIN_D5    29
#define PIN_D6    30
#define PIN_D7    31

#define PIN_E0    32
#define PIN_E1    33
#define PIN_E2    34
#define PIN_E3    35
#define PIN_E4    36
#define PIN_E5    37
#define PIN_E6    38
#define PIN_E7    39

#define PIN_F0    40
#define PIN_F1    41
#define PIN_F2    42
#define PIN_F3    43
#define PIN_F4    44
#define PIN_F5    45
#define PIN_F6    46
#define PIN_F7    47

#define PIN_G0    48
#define PIN_G1    49
#define PIN_G2    50
#define PIN_G3    51
#define PIN_G4    52
#define PIN_G5    53
#define PIN_G6    54
#define PIN_G7    55

#ifdef __AVR_ATmega32U4__
#define PIN_B7_TIMER    TIMER0A, TIMER1C
#define PIN_B6_TIMER    TIMER1B, TIMER4B // OC4B
#define PIN_B5_TIMER    TIMER1A, TIMER4B // #OC4B
#define PIN_C7_TIMER    TIMER4A    // OC4A
#define PIN_C6_TIMER    TIMER3A, TIMER4A // #OC4A
#define PIN_D7_TIMER    TIMER4D // OC4D
#define PIN_D6_TIMER    TIMER4D // #OC4D
#define PIN_D0_TIMER    TIMER0B
#endif

#endif
