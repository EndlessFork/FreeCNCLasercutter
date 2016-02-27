/*
  laser.cpp - Laser control library for Arduino using 16 bit timers- Version 1
  Copyright (c) 2013 Timothy Schmidt.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "platform.h"
#include "laser.h"
#include "pins.h"
#include "asm.h"

#include "time.h"
#include "hw_layout.h"
#include "hw_config.h"

#include <stdlib.h>

BUFFER(LASER_RASTERDATA, uint8_t, LASER_RASTERDATA_QUEUE_SIZE)

laser_t laser;

//~ void timer3_init(void) {
    //~ SET_OUTPUT(LASER_FIRING_PIN);
//~
    //~ TCCR3B = 0x00;  // stop Timer3 clock for register updates
    //~ TCCR3A = 0x82; // Clear OC3A on match, fast PWM mode, lower WGM3x=14
    //~ ICR3 = labs(F_CPU / LASER_PWM); // clock cycles per PWM pulse
    //~ OCR3A = labs(F_CPU / LASER_PWM) - 1; // ICR3 - 1 force immediate compare on next tick
    //~ TCCR3B = 0x18 | 0x01; // upper WGM4x = 14, clock sel = prescaler, start running
//~
    //~ CRITICAL_SECTION_START;
    //~ TCCR3B &= 0xf8; // stop timer, OC3A may be active now
    //~ TCNT3 = labs(F_CPU / LASER_PWM); // force immediate compare on next tick
    //~ ICR3 = labs(F_CPU / LASER_PWM); // set new PWM period
    //~ TCCR3B |= 0x01; // start the timer with proper prescaler value
    //~ CRITICAL_SECTION_END;
//~ }
void timer4_init(void) {
    PLLFRQ |= (1<<PLLTM0); /* PCK 48MHz */

    // stop Timer4 clock for register updates
    TCCR4B = 0x00; // PWM4X, PSR4, DTPS41, DTPS40, CS43, CS42, CS41, CS40

    // settings are for PWM6, (single/dual) slope, output #OC4D (PD6)
    // PWM_period is set with OCR4C and duty_cycle with OCR4A
    //
    // Enhanced Mode: ENHC4=1
    // PWM6 : PWM4A=1, WGM41=1, WGM40=0(single slope)/1(dual slope)
    // #OC4D: COM4D1=1, OC4OE4=1 (set when laser fires)
    // Clocksource=PLL: CS4x=0001 (48MHz)

    TCCR4A = 0x02; // COM4A1, COM4A0, COM4B1, COM4B0, FOC4A, FOC4B, PWM4A, PWM4B
    TCCR4C = 0x08; // COM4A1S, COM4A0S, COM4B1S, COMAB0S, COM4D1, COM4D0, FOC4D, PWM4D
    #ifdef TIMER4_FASTPWM
    // single-slope PWM6
    TCCR4D = 0x02; // FPIE4, FPEN4, FPNC4, FPES4, FPAC4, FPF4, WGM41, WGM40
    #else
    // dual-slope PWM6
    TCCR4D = 0x03; // FPIE4, FPEN4, FPNC4, FPES4, FPAC4, FPF4, WGM41, WGM40
    #endif
    // enhanced mode
    TCCR4E = 0x40; // TLOCK4, ENHC4, OC4OE5, OC4OE4, OC4OE3, OC4OE2, OC4OE1, OC4OE0

    // set period for PWM6
    TC4H = (TIMER4_TICKS >> 8) & 0x3;
    OCR4C = TIMER4_TICKS & 0xff;

    // NO IRQ's!
    TIMSK4 = 0; // OCIE4D, OCIE4A, OCIE4B, -, -, TOIE4, -, -

    TCNT4 = 0;
    // start timer4
    TCCR4B = 0x01; // PWM4X, PSR4, DTPS41, DTPS40, CS43, CS42, CS41, CS40
}

void timer3_init(void) {
    // waveform generation = 0100 = CTC, output mode = 00 (disconnected), CS=010 (div by 8)
    TCCR3A = 0b00000000; // COM3A1,COM3A0,COM3B1,COM3B0,COM3C1,COM3C0,WGM31,WGM30
    TCCR3B = 0b00001000;// ICNC3,ICES3,-,WGM33,WGM32,CS32,CS31,CS30
    // CS = off, 1, 8, 64, 256, 1024, TC0, TC1
    TCNT3 = 0;
    TIMSK3 |= (1<<OCIE3A);
}

static uint32_t remaining_pulse;

// set a timeout-timer to extinguish the laser after some us
void laser_start_pulse_timer(uint32_t pulse_duration_us) {
    LOG_STRING("L: PULSing for");LOG_U32(pulse_duration_us);LOG_STRING("us\n");
    if (!pulse_duration_us) {
        // switch off
        TCCR3B = 0b00001000;
    } else if (pulse_duration_us < 4096) {
        OCR3A = 16 * pulse_duration_us;
        TCCR3B = 0b00001001; // CS=001 : div-by-1: 16 ticks per us
    } else if (pulse_duration_us < 8L * 4096) {
        OCR3A = 2 * pulse_duration_us;
        TCCR3B = 0b00001010; // CS=010 : div-by-8: 2 ticks per us
    } else if (pulse_duration_us < 64L * 4096) {
        OCR3A = pulse_duration_us / 4;
        TCCR3B = 0b00001011; // CS=011 : div-by-64: 4 us per tick
    } else if (pulse_duration_us < 256L * 4096) {
        OCR3A = pulse_duration_us / 16;
        TCCR3B = 0b00001100; // CS=100 : div-by-256: 16 us per tick
    } else if (pulse_duration_us < 1024L * 4096) {
        OCR3A = pulse_duration_us / 64;
        TCCR3B = 0b00001101; // CS=101 : div-by-1024: 64 us per tick
    } else {
        remaining_pulse = pulse_duration_us - 512*2000L;
        laser_start_pulse_timer(512*2000L);
        return;
    }
    remaining_pulse = 0;
}

ISR(TIMER3_COMPA_vect) {
    CHECK_STACK;
    if (laser.laser_is_on) {
        if (remaining_pulse) {
            laser_start_pulse_timer(remaining_pulse);
            return;
        }
    }
    TCCR3B &=~ 7; // stop timer 3 (set CS=000)
    //~ TCNT3 = 0;  // next pulse start at 0 ticks...
    laser_fire(0); // switch off laser (also disables timer3)
}

void laser_init()
{
    CLR_PIN(LASER_FIRING_PIN);  // Laser FIRING is active HIGH
    SET_OUTPUT(LASER_FIRING_PIN);

    // Initialize timers for laser intensity control
    timer4_init();
    timer3_init();

    // initialize state to some sane defaults
    laser.power = 100; // 10%
    laser.ppm = 1.0;
    laser.pulse_duration_us = 1000;
    laser.laser_is_on = 0;
    laser.mode = LASER_OFF;
    laser.last_firing = 0;
    laser.pulses = 0;
    //~ laser.raster_aspect_ratio = (int) (1000 * LASER_RASTER_ASPECT_RATIO);
    //~ laser.raster_um_per_pulse = (int) (1000 * LASER_RASTER_MM_PER_PULSE);
    laser.laser_time_ms = 59990;

    laser_fire(0);
}


void laser_fire(uint8_t intensity){ // intensity from 0..255
    uint16_t tmp;

    if (intensity) {

        if (!laser.laser_is_on)
            laser_set_last_firing(micros()); // microseconds of last laser firing
        laser_set_firing(1);

        SET_PIN(LASER_FIRING_PIN);
        if (intensity == 255) {
            TCCR4E &=~ (1<<OC4OE4);    // DISABLE PWM for #OC4D
            LOG_STRING("L: ON\n");
        } else { // intensity 1..254
            // calculate PWM setting // XXX include laser.power! (atm set externally)

            // intensity * 257 -> 0..65535 canceled by >> 16
            // TIMER4_TICKS*2 because of enhanced mode!
            tmp = mul16SHR16(intensity | (intensity << 8), 2 * TIMER4_TICKS);

            if (tmp > 2 * TIMER4_TICKS + 1) {
                tmp = 2 * TIMER4_TICKS + 1; // restrict intensity between 0 and 100%
                LOG_STRING("L: ERROR: laser_pwm above TIMER4_TICKS! " __FILE__ ":");LOG_U16(__LINE__);LOG_NEWLINE;
            }

            // PWM
            // set new duty cycle
            TC4H = (tmp >> 8) & 0x07;
            OCR4A = tmp & 0xff; // XXX:check code: maybe gcc does it right already?
            TCCR4E |= (1<<OC4OE4); // ENABLE PWM for #OC4D
            LOG_STRING("L: PWM: ");LOG_U16(tmp);LOG_CHAR('/');LOG_U16(TIMER4_TICKS);LOG_NEWLINE;
        }

    } else {
        // switch off (power=0)
        CLR_PIN(LASER_FIRING_PIN);
        TCCR4E &=~ (1<<OC4OE4);    // DISABLE PWM for #OC4D
        if (laser.laser_is_on) {
            laser_set_firing(0);
            LOG_STRING("L: OFF\n");

            laser_add_time(micros() - laser.last_firing);
            TCCR3B &=~ 7; // stop pulse timer
            TCNT3 = 0;
         }
    }
}

void laser_add_time(uint32_t us) {
    uint32_t ms=0;
    CRITICAL_SECTION_START;
    us = laser.lasing_time + us;
    laser.lasing_time = 0;
    CRITICAL_SECTION_END;
    us += laser.laser_time_us;
    while (us >= 1000) {
        us -= 1000;
        ms++;
    }
    laser.laser_time_us = us;
    ms += laser.laser_time_ms;
    while (ms >= 60000) {
        laser.laser_time_min++;
        ms -= 60000;
    }
    laser.laser_time_ms = ms;
    //~ Config_StoreSettings();
    LOG_STRING("L: total lasing time:");
    LOG_U32(laser.laser_time_min);
    LOG_STRING("minutes,");
    LOG_U32(laser.laser_time_ms);
    LOG_STRING("ms,");
    LOG_U16(laser.laser_time_us);
    LOG_STRING("us\n");
}


void laser_set_mode(uint8_t mode) {
    if (mode < 8) {
        laser.mode = mode;
        switch (mode) {
            case LASER_OFF : LOG_STRING("L: Mode is 0 (OFF)\n");break;
            case LASER_CW : LOG_STRING("L: Mode is 1 (CW)\n");break;
            case LASER_PULSE : LOG_STRING("L: Mode is 2 (PULSE)\n");break;
            case LASER_MODULATE : LOG_STRING("L: Mode is 4 (Modulate)\n");break;
            case LASER_RASTER_CW : LOG_STRING("L: Mode is 5 (RasterCW)\n");break;
            case LASER_RASTER_PULSE : LOG_STRING("L: Mode is 6 (RasterPulse)\n");break;
        }
    } else {
        LOG_STRING("L: IGNORING Illegal Mode ");LOG_U8(mode);LOG_NEWLINE;
    }
}

void laser_set_modulation(uint8_t modulation) {
    if (modulation < 8) {
        laser.modulation = modulation;
        switch (modulation) {
            case LASER_MODULATION_1BIT : LOG_STRING("L: Modulation 1Bits per Pixel\n");break;
            case LASER_MODULATION_2BIT : LOG_STRING("L: Modulation 2Bits per Pixel\n");break;
            case LASER_MODULATION_3BIT : LOG_STRING("L: Modulation 3Bits per Pixel (EXPERIMENTAL)\n");break;
            case LASER_MODULATION_4BIT : LOG_STRING("L: Modulation 4Bits per Pixel\n");break;
            case LASER_MODULATION_6BIT : LOG_STRING("L: Modulation 6Bits per Pixel (EXPERIMENTAL)\n");break;
            case LASER_MODULATION_8BIT : LOG_STRING("L: Modulation 8Bits per Pixel\n");break;
        }
    } else {
        LOG_STRING("L: IGNORING Illegal Modulation");LOG_U8(modulation);LOG_NEWLINE;
    }
}

// setters to be used by parser/gcode

void laser_set_ppm(number ppm) {
    // actually this is pulse-per-millimeter times 1000. So pulse-per-meter effectively
    LOG_STRING("L: setting PulsePerMeter: ");
    LOG_U32(ppm);
    LOG_NEWLINE;
    laser.ppm = ppm;
}

void laser_set_pulse_us(uint32_t pulse_duration_us) { // laser firing pulse_us in microseconds, for pulsed firing mode
    LOG_STRING("L: pulse_us: ");LOG_U32(pulse_duration_us);LOG_NEWLINE;
    laser.pulse_duration_us = pulse_duration_us;
}

void laser_set_last_firing(uint32_t last_firing) { // microseconds of last laser activation
    LOG_STRING("L: starting laser millis: ");LOG_U32(last_firing);LOG_NEWLINE;
    laser.last_firing = last_firing;
}

void laser_set_power(uint16_t power) { // Laser firing intensity 0-1000 (0..100%)
    if (power <= 1000) {
        LOG_STRING("L: set power: ");LOG_U16(power);LOG_NEWLINE;
        laser.power = power;
    } else
        LOG_STRING("L: set power: ILLEGAL VALUE FOR POWER:");LOG_U16(power);LOG_NEWLINE;
}

void laser_set_firing(uint8_t firing) {// LASER_ON / LASER_OFF - current state
    if (firing) {
        LOG_STRING("L: set firing ON\n");
        laser.laser_is_on = 1;
    } else {
        LOG_STRING("L: set firing OFF\n");
        laser.laser_is_on = 0;
    }
}

void laser_set_raster_num_pixels(uint16_t raster_num_pixels) {
    LOG_STRING("L: RASTER_num_pixels: ");LOG_U16(raster_num_pixels);LOG_NEWLINE;
    laser.raster_num_pixels = raster_num_pixels;
}


#ifdef LASER_PERIPHERALS
bool laser_peripherals_ok(){
    return !digitalRead(LASER_PERIPHERALS_STATUS_PIN);
}
void laser_peripherals_on(){
    digitalWrite(LASER_PERIPHERALS_PIN, LOW);
    if (laser.diagnostics) {
      SERIAL_ECHO_START;
      SERIAL_ECHOLNPGM("Laser Peripherals Enabled");
    }
}
void laser_peripherals_off(){
    if (!digitalRead(LASER_PERIPHERALS_STATUS_PIN)) {
      digitalWrite(LASER_PERIPHERALS_PIN, HIGH);
      if (laser.diagnostics) {
        SERIAL_ECHO_START;
        SERIAL_ECHOLNPGM("Laser Peripherals Disabled");
      }
    }
}
void laser_wait_for_peripherals() {
    uint32_t timeout = millis() + LASER_PERIPHERALS_TIMEOUT;
    if (laser.diagnostics) {
      SERIAL_ECHO_START;
      SERIAL_ECHOLNPGM("Waiting for peripheral control board signal...");
    }
    while(!laser_peripherals_ok()) {
        if (millis() > timeout) {
            if (laser.diagnostics) {
              SERIAL_ERROR_START;
              SERIAL_ERRORLNPGM("Peripheral control board failed to respond");
            }
            Kill();
            break;
        }
    }
}
#endif // LASER_PERIPHERALS
