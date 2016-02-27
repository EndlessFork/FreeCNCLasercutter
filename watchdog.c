

#include "platform.h"
#include "watchdog.h"
#ifdef USE_WATCHDOG
#include <avr/wdt.h>
#include "lcd.h"

//~ #define wdt_reset() volatile asm("wdr")

/// initalise watchdog with a 4 sec interrupt time
void watchdog_init() {
#ifdef WATCHDOG_RESET_MANUAL
    //We enable the watchdog timer, but only for the interrupt.
    //Take care, as this requires the correct order of operation, with interrupts disabled. See the datasheet of any AVR chip for details.
    CRITICAL_SECTION_START;
    wdt_reset();
    _WD_CONTROL_REG = _BV(_WD_CHANGE_BIT) | _BV(WDE);
    _WD_CONTROL_REG = _BV(WDIE) | WDTO_4S;
    CRITICAL_SECTION_END;
#else
    wdt_enable(WDTO_4S);
#endif
}


//===========================================================================
//=============================ISR               ============================
//===========================================================================

//Watchdog timer interrupt, called if main program blocks >1sec and manual reset is enabled.
#ifdef WATCHDOG_RESET_MANUAL
ISR(WDT_vect)
{
    lcd_print_at(0,0,PSTR("  WATCHDOG ERROR"));
    lcd_print_at(1,0,PSTR("PLEASE RESET !"));
    kill(); //kill blocks
}
#endif//RESET_MANUAL

#endif//USE_WATCHDOG
