/*
  laser.h - Laser cutter control library for Arduino using 16 bit timers- Version 1
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

#ifndef LASER_H
#define LASER_H

#include <inttypes.h>
#include "hw_config.h"
#include "platform.h"
#include "buffer.h"

BUFFER_H(LASER_RASTERDATA, uint8_t, LASER_RASTERDATA_QUEUE_SIZE)

// split into planned and status
typedef struct {
    // accumulate lasing time here:
    uint32_t lasing_time;
    uint16_t laser_time_us;
    uint16_t laser_time_ms;
    uint32_t laser_time_min;

    uint24_t    ppm; // pulses per meter, in PULSED firing mode
    uint24_t    pulse_duration_us; // pulse duration in us, for PULSED firing mode
    uint32_t    last_firing; // microseconds since last laser firing
    uint8_t     raster_num_pixels; // 8 bits ought to be enough for 0..63 pixels (LASER_MAX_RASTER_LINE)
    uint16_t    power:10; // Laser firing POWER 0-1000 (0..100%)
    uint8_t     mode:3;
    uint8_t     modulation:3; // 0=BW, 1=2bits, 3=4Bits, 7=8Bits
    bool        laser_is_on; // LASER_ON / LASER_OFF - current state
    uint32_t    pulses;
} laser_t;

extern laser_t laser;

// laser mode mask: need 3 Bits
#define LASER_OFF           0
#define LASER_CW            1
#define LASER_PULSE         2
#define LASER_MODULATE      4
#define LASER_RASTER_CW     5
#define LASER_RASTER_PULSE  6

// laser modulation bitdepth: need 3 Bits
#define LASER_MODULATION_1BIT   0
#define LASER_MODULATION_2BIT   1
#define LASER_MODULATION_3BIT   2
#define LASER_MODULATION_4BIT   3
#define LASER_MODULATION_6BIT   5
#define LASER_MODULATION_8BIT   7


void laser_init(void);
void laser_update_lifetime(void);

void laser_fire(uint8_t intensity);

void laser_start_pulse_timer(uint32_t pulse_us);
void laser_add_time(uint32_t us);
void laser_set_mode(uint8_t mode);
void laser_set_ppm(number ppm);
void laser_set_pulse_us(uint32_t pulse_us);
void laser_set_last_firing(uint32_t last_firing);
void laser_set_power(uint16_t power);
void laser_set_mode(uint8_t mode);
void laser_set_modulation(uint8_t modulation);
void laser_set_firing(uint8_t firing);
void laser_set_raster_num_pixels(uint16_t raster_num_pixels);

#ifdef LASER_PERIPHERALS
  bool laser_peripherals_ok(void);
  void laser_peripherals_on(void);
  void laser_peripherals_off(void);
  void laser_wait_for_peripherals(void);
#endif // LASER_PERIPHERALS

#endif // LASER_H
