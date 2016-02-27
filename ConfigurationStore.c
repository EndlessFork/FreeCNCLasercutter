#include "ConfigurationStore.h"

#include "hw_config.h"

#ifdef EEPROM_SETTINGS

#define NUM_SLOTS        ((E2END-2) / (1+sizeof(Configuration)))
#define MAX_SLOT        (NUM_SLOTS-1)
#define SLOT_ADDR(X)    (2+NUM_SLOTS*(((uint16_t)X)*sizeof(Configuration)))

// Layout of the EEPROM is as follows: (this struct is used nowhere)
struct EEPROM_LAYOUT {
    uint8_t    num_slots;
    uint8_t revision;
    uint8_t serials[NUM_SLOTS];
    Configuration configs[NUM_SLOTS];
};

/* find_slot() finds the most recent slot (0..NUM_SLOTS-1). it may return -1 if there is no such thing
 * Config_Retrieve gets the most recent stored data or calls Config_ResetDefault if there is no data
 * Config_StoreSetting stores the current Config
 */

int16_t find_slot(void)
{    uint8_t revision, length, i, slot, serial;
    // wait until eeprom idle;
    cli();
    eeprom_busy_wait();
    sei();
    // query num_slots and revision and check against known values
    cli();i=eeprom_read_byte((uint8_t*) 0);eeprom_busy_wait();sei();
    cli();revision=eeprom_read_byte((uint8_t*) 1);eeprom_busy_wait();sei();
    if ((i != NUM_SLOTS) || (revision != CONFIGURATION_REVISION))
        return -1;
    // check first entry's metadata
    cli();i=eeprom_read_byte((uint8_t*) 2);sei();
    if (i==0xff) {      // first location not yet used
        return -1;
    }
    cli();revision=eeprom_read_byte((uint8_t*) SLOT_ADDR(0));eeprom_busy_wait();sei();
    cli();length=eeprom_read_byte((uint8_t*) (SLOT_ADDR(0)+1));eeprom_busy_wait();sei();
    if ((length != sizeof(Config)) || (revision != CONFIGURATION_REVISION))
        return -1;

    // scan eeprom for the most recent slot (follow the chain...)
    cli();serial=eeprom_read_byte((uint8_t*) 2 + MAX_SLOT);eeprom_busy_wait();sei();
    for(slot=0;slot<NUM_SLOTS;slot++)
    {
        // normally the serial for slot_i is one bigger than for slot_(i-1)
        // if not, slot_i is the oldest and slot_(i-1)
        // a cleared eeprom has all bytes=0xFF => serials MUST be !=0xff (here: serials are 0..0x7F)
        // slots with illegal serials are ignored
        cli();i=eeprom_read_byte((uint8_t*) 2 + slot);eeprom_busy_wait();sei();
        if ((i != ((serial + 1) & 0x7F)) && (serial <= 0x7F))
            if (slot)
                return slot-1;
            return MAX_SLOT;
        serial = i;
    }
    // not found
    return -1;
}

void Config_RetrieveSettings(void)
{    int16_t slot = find_slot();
    if (slot == -1)
    {
        // no valid data in eeprom, get valid data
        Config_ResetDefault();
        return;
    }
    cli();
    eeprom_busy_wait();
    eeprom_read_block(&Config,(uint8_t*) (uint16_t)SLOT_ADDR(slot), sizeof(Configuration));
    eeprom_busy_wait();
    sei();
};

// Config_StoreSettings
void Config_StoreSettings(void)
{    int16_t    slot = find_slot();
    uint8_t    serial;
    if (slot == -1)
    {    uint8_t i;
        // init eeprom
        for(i=0; i<NUM_SLOTS; i++)
        {
            cli();
            eeprom_update_byte((uint8_t*) 2+i, 0xFF); // invalidate all SLOTS
            eeprom_busy_wait();
            sei();
        }
        eeprom_update_byte((uint8_t*) 0, NUM_SLOTS);
        eeprom_busy_wait();
        eeprom_update_byte((uint8_t*) 1, CONFIGURATION_REVISION);
        eeprom_busy_wait();
        sei();
        serial = 0;
    }
    else
    {
        cli();serial=eeprom_read_byte((uint8_t*) 2 + slot);eeprom_busy_wait();sei();serial++;
    }
    slot++;    // store to next avail. slot
    if (slot > MAX_SLOT)
        slot -= NUM_SLOTS;

    // provide valid values
    Config._revision = CONFIGURATION_REVISION;
    Config._length = sizeof(Config);
    // store data
    cli();
    eeprom_busy_wait();
    eeprom_update_block(&Config,(uint8_t*) (uint16_t) SLOT_ADDR(slot), sizeof(Configuration));
    // update serial
    eeprom_busy_wait();
    eeprom_update_byte( (uint8_t*) (uint16_t) 2+slot, serial);
    eeprom_busy_wait();
    sei();
}
#endif // EPROMSETTINGS

void Config_PrintSettings()
{  // Always have this function, even with EEPROM_SETTINGS disabled, the current values will be shown
#if 0
#ifndef DISABLE_M503
    SERIAL_ECHO_START;
    SERIAL_ECHOLNPGM("Steps per unit:");
    SERIAL_ECHO_START;
    SERIAL_ECHOPAIR("  M92 X",Config.steps_per_unit[0]);
    SERIAL_ECHOPAIR(" Y",Config.steps_per_unit[1]);
    SERIAL_ECHOPAIR(" Z",Config.steps_per_unit[2]);
    SERIAL_ECHOPAIR(" E",Config.steps_per_unit[3]);
    SERIAL_ECHOLN("");

    SERIAL_ECHO_START;
    SERIAL_ECHOLNPGM("Maximum feedrates (mm/s):");
    SERIAL_ECHO_START;
    SERIAL_ECHOPAIR("  M203 X",Config.max_speed[0]);
    SERIAL_ECHOPAIR(" Y",Config.max_speed[1] );
    SERIAL_ECHOPAIR(" Z", Config.max_speed[2] );
    SERIAL_ECHOPAIR(" E", Config.max_speed[3]);
    SERIAL_ECHOLN("");

    SERIAL_ECHO_START;
    SERIAL_ECHOLNPGM("Maximum Acceleration (mm/s2):");
    SERIAL_ECHO_START;
    SERIAL_ECHOPAIR("  M201 X" ,Config.max_accel[0] );
    SERIAL_ECHOPAIR(" Y" , Config.max_accel[1] );
    SERIAL_ECHOPAIR(" Z" ,Config.max_accel[2] );
    SERIAL_ECHOPAIR(" E" ,Config.max_accel[3]);
    SERIAL_ECHOLN("");
    SERIAL_ECHO_START;
    SERIAL_ECHOLNPGM("Acceleration: S=Config.accel, T=retract Config.accel");
    SERIAL_ECHO_START;
    SERIAL_ECHOPAIR("  M204 S",Config.accel );
    SERIAL_ECHOPAIR(" T" ,retract_acceleration);
    SERIAL_ECHOLN("");

    SERIAL_ECHO_START;
    SERIAL_ECHOLNPGM("Advanced variables: S=Min feedrate (mm/s), T=Min travel feedrate (mm/s), B=minimum segment time (ms), X=maximum XY jerk (mm/s),  Z=maximum Z jerk (mm/s),  E=maximum E jerk (mm/s)");
    SERIAL_ECHO_START;
    SERIAL_ECHOPAIR("  M205 S",minimumfeedrate );
    SERIAL_ECHOPAIR(" T" ,mintravelfeedrate );
    SERIAL_ECHOPAIR(" B" ,minsegmenttime );
    SERIAL_ECHOPAIR(" X" ,max_xy_jerk );
    SERIAL_ECHOPAIR(" Z" ,max_z_jerk);
    SERIAL_ECHOPAIR(" E" ,max_e_jerk);
    SERIAL_ECHOLN("");

    SERIAL_ECHO_START;
    SERIAL_ECHOLNPGM("Home offset (mm):");
    SERIAL_ECHO_START;
    SERIAL_ECHOPAIR("  M206 X",add_homeing[0] );
    SERIAL_ECHOPAIR(" Y" ,add_homeing[1] );
    SERIAL_ECHOPAIR(" Z" ,add_homeing[2] );
    SERIAL_ECHOLN("");

    SERIAL_ECHO_START;
    SERIAL_ECHOLNPGM("Laser lifetime usage:");
    SERIAL_ECHO_START;
    SERIAL_ECHOPAIR(" Hours: ",(uint32_t)labs(laser.lifetime / 60));
    SERIAL_ECHOLN("");
    SERIAL_ECHOPAIR(" Minutes: ",(uint32_t)laser.lifetime % 60);
    SERIAL_ECHOLN("");

#endif
#endif
}


Configuration_t Config /*=  {
    //~ .steps_per_unit = DEFAULT_AXIS_STEPS_PER_UNIT,
    .steps_per_unit = {XY_AXIS_STEPS_PER_M, XY_AXIS_STEPS_PER_M, Z_AXIS_STEPS_PER_M},
    .max_speed = DEFAULT_MAX_SPEED,
    .max_accel = DEFAULT_MAX_ACCELERATION,
    .jerk = {DEFAULT_XY_JERK, DEFAULT_XY_JERK, DEFAULT_Z_JERK},
    .homing_speed = HOMING_SPEED,
    .tool_accel = DEFAULT_TOOL_ACCEL,
    .tool_speed = DEFAULT_TOOL_SPEED,
    .travel_accel = DEFAULT_TRAVEL_ACCEL,
    .travel_speed = DEFAULT_TRAVEL_SPEED,
    .min_speed = DEFAULT_MIN_SPEED,
    .axis_home_dir = {X_HOME_DIR, Y_HOME_DIR, Z_HOME_DIR},
    .axis_length = {X_MAX_LENGTH, Y_MAX_LENGTH, Z_MAX_LENGTH},
}*/;

Globals_t G /*= {
    //~ .accel = DEFAULT_TRAVEL_ACCEL,
    //~ .speed = DEFAULT_TRAVEL_SPEED
}*/;

void Config_ResetDefault()
{
    G.speed = 5000;
    //~ float laser_lifetime = 0.0;
}
