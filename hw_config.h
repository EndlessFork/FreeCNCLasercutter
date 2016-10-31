#ifndef _HW_CONFIG_H_
#define _HW_CONFIG_H_

/********
 * TODO *
 ********
 * convert all float constants to um_per_... or ..._per_m
 * remove unused defines/options
 */


/******************************
 * Firmware specific settings *
 ******************************/

// User-specified version info of this build to display in [Pronterface, etc] terminal window during
// startup.
#define STRING_VERSION_CONFIG_H __DATE__ " " __TIME__ // build date and time
#define STRING_CONFIG_H_AUTHOR "(none, default config)" // Who made the changes.

// Firmware name
#define CUSTOM_FIRMWARE_NAME "Freetronics CNCPlotter Lasercutter"

// Should ASM optimizations be used?
#define OPTIMIZE_ASM

// should settings be stored to EEPROM?
//~ #define EEPROM_SETTINGS

#include "hw_layout.h"

// Experimental!
#define USE_MCP4922

/***********************
 * Mechanical Settings *
 ***********************/

// China Town K40 CO2 Laser Engraver/Cutter
#define X_MAX_POS 360
#define X_MIN_POS -9

#define Y_MAX_POS 241
#define Y_MIN_POS -5

#define Z_MAX_POS 75
#define Z_MIN_POS -75

#define X_MAX_LENGTH (X_MAX_POS - X_MIN_POS)
#define Y_MAX_LENGTH (Y_MAX_POS - Y_MIN_POS)
#define Z_MAX_LENGTH (Z_MAX_POS - Z_MIN_POS)

//// MOVEMENT SETTINGS
#define NUM_AXIS 3 // The axis order in all axis related arrays is X, Y, Z...
#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2
#define L_AXIS 3
#define HOMING_SPEED 10  // set the homing speed (mm/s)

// Laser Cutter
#define XY_AXIS_STEPS_PER_M           79220
#define Z_AXIS_STEPS_PER_M            800000
//~ #define DEFAULT_AXIS_STEPS_PER_UNIT   {79.22, 79.22, 200.0}  // default steps per unit (mm)
#define DEFAULT_MAX_SPEED             {500, 500, 10}    // (mm/sec)
#define DEFAULT_MAX_ACCELERATION      {2600, 2600, 2}    // X, Y, Z maximum acceleration

#define DEFAULT_ACCELERATION          2000    // X, Y, Z max Config.accel in mm/s^2 for printing moves

// The speed change that does not require Config.accel (i.e. the software might assume it can be done instantaneously)
#define DEFAULT_XY_JERK               5.0    // (mm/sec)
#define DEFAULT_Z_JERK                0.4    // (mm/sec)


#define QUICK_HOME  //if this is defined, if both x and y are to be homed, a diagonal move will be performed initially.

#define MAX_STEP_FREQUENCY 8000 // Max stepping frequency

// neutral basic speed settings in mm/s
#define DEFAULT_TOOL_SPEED       10
#define DEFAULT_TRAVEL_SPEED     100
#define DEFAULT_TOOL_ACCEL       100
#define DEFAULT_TRAVEL_ACCEL     100


// never go slower than this in um/s or the stepper ticks overflow!...
// 46340 is 65535 /sqrt(2)
#define ABS_MIN_SPEED   (F_CPU / ((46340 * XY_AXIS_STEPS_PER_M) / 1000000UL))
#define DEFAULT_MIN_SPEED        5000
#if ABS_MIN_SPEED > DEFAULT_MIN_SPEED
#warning "DEFAULT_MIN_SPEED TOO SLOW, clamping to ABS_MIN_SPEED"
#undef DEFAULT_MIN_SPEED
#define DEFAULT_MIN_SPEED ABS_MIN_SPEED
#endif
// ABS_MIN_SPEED = 4359

// The hardware watchdog should reset the Microcontroller disabling all outputs, in case the firmware gets stuck.
//#define USE_WATCHDOG


/***********
 * BUFFERS *
 ***********/

// The number of motions that can be in the plan at any give time.
// THE BLOCK_BUFFER_SIZE NEEDS TO BE A POWER OF 2, i.g. 8,16,32 because shifts and ors are used to do the ringbuffering.
#define STEPPER_QUEUE_SIZE BUFFER_SIZE_32 // needs to be smaller for SD support!

// Size of UART RX Buffer
#define UART_RX_BUFFER_SIZE BUFFER_SIZE_256

// SIZE OF UART TX BUFFER
#define UART_TX_BUFFER_SIZE BUFFER_SIZE_32

//// Raster mode enables the laser to etch bitmap data at high speeds.  Increases command buffer size substantially.
#define LASER_RASTERDATA_QUEUE_SIZE BUFFER_SIZE_64

/****************
 * Connectivity *
 ****************/

// SERIAL_PORT selects which serial port should be used for communication with the host.
// This allows the connection of wireless adapters (for instance) to non-default port pins.
//~ #define SERIAL_PORT_USB
#define SERIAL_PORT_UART

// This determines the communication speed of the UART
// dont set too fast, or chars will get lost! (the stepper IRQ blocks the uart for upto ~ 1800 cycles ! => max 8K chars/s => max 80000 baud)
#define BAUDRATE 57600


/******************
 * LASER Settings *
 ******************/

//// The following define selects how to control the laser.  Please choose the one that matches your setup.
// 1 = Single pin control - LOW when off, HIGH when on, PWM to adjust intensity
// 2 = Two pin control - A firing pin for which LOW = off, HIGH = on, and a seperate intensity pin which carries a constant PWM signal and adjusts duty cycle to control intensity
#define LASER_CONTROL 1

// pixels aren't square on most displays, 1.33 == 4:3 aspect ratio
#define LASER_RASTER_ASPECT_RATIO 1

// maximum number of base64 encoded pixels per raster gcode command (<= 63 !!!)
#define LASER_MAX_RASTER_LINE 63

// Can be overridden by providing an R value in M649 command : M649 S17 B2 D0 R0.1 F4000
#define LASER_RASTER_MM_PER_PULSE 0.2

//// Uncomment the following if the laser cutter is equipped with a peripheral relay board
//// to control power to an exhaust fan, water pump, laser power supply, etc.
//#define LASER_PERIPHERALS
//#define LASER_PERIPHERALS_TIMEOUT 30000  // Number of milliseconds to wait for status signal from peripheral control board

//// Uncomment the following line to enable cubic bezier curve movement with the G5 code
//#define G5_BEZIER

// Just for information: Maximum power in watts
#define LASER_WATTS 40

// Just for information: expected (focussed) beam size in um
#define LASER_DIAMETER 100

// Frequency to be used for PWM modulation of control signal
#define LASER_PWM 50000

// z axis position at which the laser is focused (in um)
#define LASER_FOCAL_HEIGHT 74500

// TIMER4 is used for PWM modulation of laser control signal.
// Choose if FASTPWM shjould be used or normal PWM
#define TIMER4_FASTPWM
#ifdef TIMER4_FASTPWM
    #define TIMER4_TICKS ((F_TIMER4/LASER_PWM)-1)
    #if TIMER4_TICKS > 1023
        #undef TIMER4_TICKS
        #define TIMER4_TICKS 1023
        #undef LASER_PWM
        #define LASER_PWM (F_TIMER4/(TIMER4_TICKS+1))
    #endif
#else
    #define TIMER4_TICKS ((F_TIMER4/2*LASER_PWM)-1)
    #if TIMER4_TICKS > 1023
        #undef TIMER4_TICKS
        #define TIMER4_TICKS 1023
        #undef LASER_PWM
        #define LASER_PWM (F_TIMER4/(2*(TIMER4_TICKS+1)))
    #endif
#endif


/**********
 * Extras *
 **********/

// Size of LCD Display
#define LCD_WIDTH 20
#define LCD_HEIGHT 4

/*******************
 * AUTOMATIC STUFF *
 *******************/

// Derive init sequence for laser firing stuff
#ifdef LASER_INTENSITY_PIN
    #define INIT_LASER_FIRE    SET_OUTPUT(LASER_FIRING_PIN);CLR_PIN(LASER_FIRING_PIN);SET_OUTPUT(LASER_INTENSITY_PIN);CLR_PIN(LASER_INTENSITY_PIN)
#else
    #define INIT_LASER_FIRE    SET_OUTPUT(LASER_FIRING_PIN);CLR_PIN(LASER_FIRING_PIN)
#endif

// Derive Homing direction from defined limit switches
#ifdef X_MIN_PIN
    #define X_HOME_DIR -1
#else
    #ifdef X_MAX_PIN
        #define X_HOME_DIR 1
    #else
        #define X_HOME_DIR 0
    #endif
#endif
#ifdef Y_MIN_PIN
    #define Y_HOME_DIR -1
#else
    #ifdef Y_MAX_PIN
        #define Y_HOME_DIR 1
    #else
        #define Y_HOME_DIR 0
    #endif
#endif
#ifdef Z_MIN_PIN
    #define Z_HOME_DIR -1
#else
    #ifdef Z_MAX_PIN
        #define Z_HOME_DIR 1
    #else
        #define Z_HOME_DIR 0
    #endif
#endif

// Derive Reference Postion from Homing direction
#if X_HOME_DIR == -1
    #define X_HOME_POS (X_MIN_POS)
#else
    #define X_HOME_POS (X_MAX_POS)
#endif //X_HOME_DIR == -1
#if Y_HOME_DIR == -1
    #define Y_HOME_POS (Y_MIN_POS)
#else
    #define Y_HOME_POS (Y_MAX_POS)
#endif //Y_HOME_DIR == -1

#if Z_HOME_DIR == -1
    #define Z_HOME_POS (Z_MIN_POS)
#else
    #define Z_HOME_POS (Z_MAX_POS)
#endif //Z_HOME_DIR == -1



// Derive Laser Peripherals init sequence
#ifdef LASER_PERIPHERALS_PIN
    #ifdef LASER_PERIPHERALS_STATUS_PIN
        #define INIT_LASER_PERIPH SET_OUTPUT(LASER_PERIPHERALS_PIN);CLR_PIN(LASER_PERIPHERALS_PIN);SET_PULLUP(LASER_PERIPHERALS_STATUS_PIN)
    #else
        #define INIT_LASER_PERIPH SET_OUTPUT(LASER_PERIPHERALS_PIN);CLR_PIN(LASER_PERIPHERALS_PIN)
    #endif
#else
    #define INIT_LASER_PERIPH
#endif

// Derive total Peripheral init sequence
#define INIT_LASER    INIT_LASER_FIRE;INIT_LASER_PERIPH
#define INIT_ALL    INIT_X;INIT_Y;INIT_Z;INIT_LASER;SET_OUTPUT(UART_TX_PIN);SET_PULLUP(UART_RX_PIN);

#endif
