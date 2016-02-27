#ifndef WATCHDOG_H
#define WATCHDOG_H



#ifdef USE_WATCHDOG
    // initialise watch dog with a 1 sec interrupt time
    void watchdog_init(void);
    // pad the dog/reset watchdog. MUST be called at least every second after the first watchdog_init or avr will go into emergency procedures..
/// reset watchdog. MUST be called every 1s after init or avr will reset.
    inline void watchdog_reset(void) { asm volatile ("wdr"); }
#else
    //If we do not have a watchdog, then we can have empty functions which are optimized away.
    #define watchdog_init() {}
    #define watchdog_reset() {}
#endif

#endif
