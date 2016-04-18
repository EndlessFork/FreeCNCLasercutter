
#include "time.h"

static volatile uint32_t    _micros;
static volatile uint8_t        _micros_ctr;
static volatile uint32_t    _millis;
volatile uint8_t    time_flags;

FORCE_INLINE uint32_t millis(void) {
    return _millis;
}
FORCE_INLINE uint32_t micros(void) {
    uint32_t m;
    CRITICAL_SECTION_START;
    m = TCNT0 >> 1; // TCNT0 advances every 0.5us
    m += _micros;
    CRITICAL_SECTION_END;
    return m;
}

static uint8_t mscounter1ms;
static uint8_t mscounter10ms;
static uint8_t mscounter100ms;
static uint8_t mscounter1s;
static uint8_t mscounter10s;
static uint8_t mscounter1m;
static uint8_t mscounter10m;

void time_init()
{
    _micros = 0;
    _micros_ctr = 0;
    _millis = 0;
    // waveform generation WGM = 010 = CTC, no OCR0x outputs used
    // Clockdiv = 8, CS = 010
    TCCR0A = 0b00000010; // COM0A1,COM0A0,COM0B1,COM0B0,-,-,WGM01,WGM00
    TCCR0B = 0b00000010; // FOC0A,FOC0B,-,-,WGM02,CS02,CS01,CS00
    // CS = off, 1, 8, 64, 256, 1024, TC0, TC1

    OCR0A = 250-1; // 125 us -> overflows at 8KHz
    TCNT0 = 0;
    TIMSK0 = (1<<OCIE0A); // -,-,-,-,-,OCIE0B,OCIE0A,TOIE0
    mscounter1ms = 10;
    mscounter10ms = 10;
    mscounter100ms = 10;
    mscounter1s = 10;
    mscounter10s = 6;
    mscounter1m = 10;
    mscounter10m = 6;
}
#define TIME_FLAG_1MS    1
#define TIME_FLAG_10MS    2
#define TIME_FLAG_100MS    4
#define TIME_FLAG_1S    8
#define TIME_FLAG_10S    16
#define TIME_FLAG_1MIN    32
#define TIME_FLAG_10MIN    64
#define TIME_FLAG_1H    128


ISR(TIMER0_COMPA_vect)
{
    ACTIVE_IRQ_1;
    CHECK_STACK;
    _micros += 125;
    if (++_micros_ctr >= 8){
        _micros_ctr -= 8;
        _millis++;
        time_flags |= TIME_FLAG_1MS;
        if (!--mscounter1ms) {
            time_flags |= TIME_FLAG_10MS;
            mscounter1ms = 10;
            if(!--mscounter10ms) {
                time_flags |= TIME_FLAG_100MS;
                mscounter10ms = 10;
                if (!--mscounter100ms) {
                    time_flags |= TIME_FLAG_1S;
                    mscounter100ms = 10;
                    if(!--mscounter1s) {
                        time_flags |= TIME_FLAG_10S;
                        mscounter1s = 10;
                        if (!--mscounter10s) {
                            time_flags |= TIME_FLAG_1MIN;
                            mscounter10s = 6;
                            if (!--mscounter1m) {
                                time_flags |= TIME_FLAG_10MIN;
                                mscounter1m = 10;
                                if(!--mscounter10m) {
                                    time_flags |= TIME_FLAG_1H;
                                    mscounter10m = 6;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    ACTIVE_IRQ_NONE;
}
