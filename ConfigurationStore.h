#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include "hw_layout.h"
#include "platform.h"

// IMPORTANT:  Whenever there are changes made to the struct stored in EEPROM
// also increment the revision number. This makes sure that the default
// values are used whenever there is a change to the data.
#define CONFIGURATION_REVISION    0x26


// CONFIGURATION! (stuff to survive power off and temporary changes)

// The Structure holding the date managed inside the eeprom
// If adding fields, don't forget to set them to sensible defaults in
// Config_RestoreDefaults() !
typedef struct {
    uint8_t _revision;  // Check if we can parse the struct
    uint8_t _length;    // size of this struct in Bytes
    uint16_t laser_time_min; // in minutes
    uint16_t laser_time_ms;    // in ms (60000 ms = 1 min)
    uint16_t laser_time_us;    // in us (1000 us = 1 ms) // 10 bits may be enough for storage... overflow is handled elsewhere
    number steps_per_unit[NUM_AXIS]; // pulses per mm
    number max_accel[NUM_AXIS];    // mm/s^2 // Use M201 to override by software
    number max_speed[NUM_AXIS];    // mm/s // M203 max feedrate mm/sec
    number jerk[NUM_AXIS];
    number homed_position[NUM_AXIS];
    number homing_speed;

    int8_t axis_home_dir[NUM_AXIS];
    number axis_length[NUM_AXIS];

    // G0 fast moving
    number travel_accel;
    number travel_speed;
    // Gx normal moving
    number tool_accel;
    number tool_speed;

    number min_speed;
} Configuration_t;

extern Configuration_t Config;    // THE one instance to use

// Current working state (variables not important enough to be in Config, but which are not local to a module)
// typically stuff modified by instructions

typedef struct {
    uint32_t lasing_time;    // in us (1000 us = 1 ms) // enough overflow bits needed! (enough for more than 1h continous lasing...)
    number current_position[NUM_AXIS];
    number destination[NUM_AXIS];

    uint16_t laser_intensity; // 0..1000 = 0..100.0% of maximum power

    number speed; // mm per s

} Globals_t;

extern Globals_t G;    // THE one instance to use


#ifdef EEPROM_SETTINGS
void Config_StoreSettings(void);
void Config_RetrieveSettings(void);
#else
#define Config_RetrieveSettings Config_ResetDefault
#define Config_StoreSettings()    {}
#endif

void Config_ResetDefault(void);
void Config_PrintSettings(void);

#endif
