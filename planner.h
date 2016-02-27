

#ifndef _PLANNER_H_
#define _PLANNER_H_

#include "hw_config.h"
#include "stepper.h"
#include "platform.h"
#include "buffer.h"
#include "laser.h"

#include <math.h>


typedef struct {
    // all values in um
    StepperPos position[NUM_AXIS]; // PLANNING position! (we plan starting from this position in um)
    StepperPos target[NUM_AXIS];   // PLANNING target    (moves go to this position in um)
    StepperPos arc_center_x;
    StepperPos arc_center_y;
    //~ StepperPos arc_radius;
    //~ uint16_t turns; // XXX: for multi-turn arcs
    uint32_t pulses; // for pulse/raster stuff
} planner_t; // 36 Bytes

extern planner_t PL;


// Initialize the motion plan subsystem
void planner_init(void);


// XXX
void planner_move(number speed);
void planner_line(number speed);
void planner_arc(number speed, bool ccw);

// Set position. Used for G92 instructions.
void planner_set_position(number x, number y, number z);
void planner_home_axes(uint8_t axis_mask);


extern number target[NUM_AXIS];


#endif
