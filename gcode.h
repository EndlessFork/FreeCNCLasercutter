#ifndef _GCODE_H_
#define _GCODE_H_

#include "platform.h"
#include "parser.h"
#include "planner.h"

// collect all information needed to properly handle G-Codes
typedef struct {
    number  destination[NUM_AXIS];
    number  arc_center_x;
    number  arc_center_y;
    //~ number  homed_position[NUM_AXIS];
    //~ bool    has_axis_homed[NUM_AXIS];
    //~ number  offset[NUM_AXIS];

    // currently active settings
    //~ number  accel;        // mm/s^2
    number  speed;        // mm/s
    //~ uint8_t tool;       // which 'tool'-setting to use
    bool relative_mode;  //Determines Absolute or Relative Coordinates
    bool axis_relative_mode[NUM_AXIS];

    number delay; // ms to be waited in G4
} gcode_t;

void process_command(void);

#endif
