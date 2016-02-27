
#include "platform.h"
#include "stepper.h"
#include "planner.h"
#include "hw_layout.h"
#include "main.h"
#include "asm.h"
#include "arc.h"
#include "coordinates.h"

#include <stdlib.h>


/*********************************************
 * The Planner:
 * ------------
 * Gets called from gcode.c with target coordinates in PL.target
 * the planner puts Elements into the buffer which are handled by stepper.c
 * the planner operates on steps
 *********************************************/


planner_t PL;


void planner_init(void) {
    memset(&PL, 0, sizeof(PL));
}


void planner_move(number speed) {
    planner_t *p = &PL;
    block_t *b;
    b = &(STEPPER_QUEUE_data[STEPPER_QUEUE_head]);

    speed++; // silence stupid warning!
    LOG_STRING("planner:planner_move()\n");

    LOG_STRING("MOVE from ");LOG_S32(p->position[0]);LOG_S32(p->position[1]);LOG_S32(p->position[2]);LOG_NEWLINE;
    LOG_STRING("MOVE  to  ");LOG_S32(p->target[0]);LOG_S32(p->target[1]);LOG_S32(p->target[2]);LOG_NEWLINE;

    b->line.steps[X_AXIS] = abs(p->target[X_AXIS] - p->position[X_AXIS]);
    b->line.steps[Y_AXIS] = abs(p->target[Y_AXIS] - p->position[Y_AXIS]);
    b->line.steps[Z_AXIS] = abs(p->target[Z_AXIS] - p->position[Z_AXIS]);

    b->line.direction_bits = ((p->position[X_AXIS] > p->target[X_AXIS])?1:0) |
                            ((p->position[Y_AXIS] > p->target[Y_AXIS])?2:0) |
                            ((p->position[Z_AXIS] > p->target[Z_AXIS])?4:0);
    b->job = STEPPER_MOVE_TO;
    b->base_ticks = 0xffff; // atm. ignored anyway.

    LOG_STRING("STEPPER_QUEUE_push(");
    LOG_STRING("\n steps_x = ");LOG_U24(b->line.steps[X_AXIS]);
    LOG_STRING("\n steps_y = ");LOG_U24(b->line.steps[Y_AXIS]);
    LOG_STRING("\n steps_z = ");LOG_U24(b->line.steps[Z_AXIS]);
    LOG_STRING("\n dirbits = ");LOG_U8(b->line.direction_bits);
    LOG_NEWLINE;

    b->laser.mode = LASER_OFF;

    STEPPER_QUEUE_push();

    p->position[0] = p->target[0];
    p->position[1] = p->target[1];
    p->position[2] = p->target[2];
}

// speed means mm_per_s. since it is given as number, its value is um_per_s
void planner_line(number speed) {
    planner_t *p = &PL;
    block_t *b;
    uint32_t line_length_um = length2(X_steps2position(p->position[0]-p->target[0]),Y_steps2position(p->position[1]-p->target[1]));

    b = &(STEPPER_QUEUE_data[STEPPER_QUEUE_head]);

    //~ speed++; // silence stupid warning!
    LOG_STRING("planner:planner_line()\n")

    LOG_STRING("LINE from");LOG_S32(p->position[0]);LOG_S32(p->position[1]);LOG_S32(p->position[2]);LOG_NEWLINE;
    LOG_STRING("LINE  to ");LOG_S32(p->target[0]);LOG_S32(p->target[1]);LOG_S32(p->target[2]);LOG_NEWLINE;
    LOG_STRING("SPEED");LOG_U32(speed);LOG_NEWLINE;

    b->line.steps[X_AXIS] = abs(p->target[X_AXIS] - p->position[X_AXIS]);
    b->line.steps[Y_AXIS] = abs(p->target[Y_AXIS] - p->position[Y_AXIS]);
    b->line.steps[Z_AXIS] = abs(p->target[Z_AXIS] - p->position[Z_AXIS]);

    b->line.direction_bits = ((p->position[X_AXIS] > p->target[X_AXIS])?1:0) |
                            ((p->position[Y_AXIS] > p->target[Y_AXIS])?2:0) |
                            ((p->position[Z_AXIS] > p->target[Z_AXIS])?4:0);

    //~ F_CPU * line_length_um / um_per_s = total ticks
    //~ ticks_per_major_step = total ticks / num_major_steps = F_CPU * line_length_um / (um_per_s * num_major_steps);
    b->base_ticks = F_CPU * ((uint64_t) line_length_um) / (((uint64_t) speed) * ((uint64_t) (max(b->line.steps[X_AXIS], b->line.steps[Y_AXIS]))));
    //~ b->base_ticks = 40404L * length2(b->line.steps[X_AXIS], b->line.steps[Y_AXIS]) / max(b->line.steps[X_AXIS], b->line.steps[Y_AXIS]);
    if (b->base_ticks < 2000)
        LOG_STRING("STEPS TOO FAST");

    b->job = STEPPER_LINE;

    LOG_STRING("STEPPER_QUEUE_push(");
    LOG_STRING("\n steps_x = ");LOG_U24(b->line.steps[X_AXIS]);
    LOG_STRING("\n steps_y = ");LOG_U24(b->line.steps[Y_AXIS]);
    LOG_STRING("\n steps_z = ");LOG_U24(b->line.steps[Z_AXIS]);
    LOG_STRING("\n   ticks = ");LOG_X16(b->base_ticks);
    LOG_STRING("\n dirbits = ");LOG_U8(b->line.direction_bits);
    LOG_NEWLINE;

    // set laser options directly into QUEUE, buffer_arc will push this then
    // XXX: optimize this, mul may overflow!
    b->laser.steps = (laser.pulses) ? laser.pulses : \
        (line_length_um * laser.ppm / 1000000); // ppm is a number: real value times 1000
    LOG_STRING("planner:line_length=");LOG_U32(line_length_um);LOG_NEWLINE;
    LOG_STRING("planner:laser.ppm=");LOG_U32(laser.ppm);LOG_NEWLINE;
    LOG_STRING("planner:laser_steps=");LOG_U24(b->laser.steps);LOG_NEWLINE;


    b->laser.pulse_duration_us = laser.pulse_duration_us;
    b->laser.power = laser.power;
    b->laser.mode = laser.mode;
    b->laser.modulation = laser.modulation;

    LOG_STRING("Laser opts:\n");
    LOG_STRING("   steps :");LOG_U24(b->laser.steps);LOG_NEWLINE;
    LOG_STRING(" pulse_us:");LOG_U24(b->laser.pulse_duration_us);LOG_NEWLINE;
    LOG_STRING("    power:");LOG_U16((uint16_t)b->laser.power);LOG_NEWLINE;
    LOG_STRING("    mode:");LOG_U8((uint8_t)b->laser.mode);LOG_NEWLINE;
    LOG_STRING("bitdepth:");LOG_U8((uint8_t)b->laser.modulation+1);LOG_NEWLINE;

    STEPPER_QUEUE_push();

    p->position[0] = p->target[0];
    p->position[1] = p->target[1];
    p->position[2] = p->target[2];
}

void planner_arc(number speed, bool ccw) {
    planner_t *p = &PL;

    LOG_STRING("planner:planner_arc(");LOG_U8(ccw);
    LOG_STRING(")\n");

    LOG_STRING("ARC from ");LOG_S32(p->position[0]);LOG_S32(p->position[1]);LOG_NEWLINE;
    LOG_STRING("ARC to ");LOG_S32(p->target[0]);LOG_S32(p->target[1]);LOG_NEWLINE;
    LOG_STRING("ARC around ");LOG_S32(p->arc_center_x);LOG_S32(p->arc_center_y);LOG_NEWLINE;
    if (ccw) LOG_STRING("ARC CCW\n") else LOG_STRING("ARC CW\n");

    // make coords relative for arc
    StepperPos fx = p->position[X_AXIS] - p->arc_center_x;
    StepperPos tx = p->target[X_AXIS] - p->arc_center_x;
    StepperPos fy = p->position[Y_AXIS] - p->arc_center_y;
    StepperPos ty = p->target[Y_AXIS] - p->arc_center_y;
    uint32_t R = max(length2(fx, fy), length2(tx, ty));

    buffer_arc(R, fx, fy, tx, ty, ccw);

    // XXX: what about Z-movements here ???
    if (p->target[Z_AXIS] != p->position[Z_AXIS])
        planner_move(speed);
}



void planner_home_axes(uint8_t axis_mask) {
    planner_t *p = &PL;
    block_t *b = &STEPPER_QUEUE_data[STEPPER_QUEUE_head];

    LOG_STRING("PL:home_axis(");LOG_U8(axis_mask);LOG_NEWLINE;
    LOG_STRING(" home_pos X:");LOG_S24(p->target[X_AXIS]);LOG_NEWLINE;
    LOG_STRING(" home_pos Y:");LOG_S24(p->target[Y_AXIS]);LOG_NEWLINE;
    LOG_STRING(" home_pos Z:");LOG_S24(p->target[Z_AXIS]);LOG_NEWLINE;

    b->ref.pos[X_AXIS] = p->target[X_AXIS];
    b->ref.pos[Y_AXIS] = p->target[Y_AXIS];
    b->ref.pos[Z_AXIS] = p->target[Z_AXIS];

    if (axis_mask) {
        // G28: referencing at least one axis
        b->ref.axis_mask = axis_mask;
        b->job = STEPPER_HOME;
    } else {
        // G92: just updating position
        b->ref.axis_mask = (X_AXIS_MASK|Y_AXIS_MASK|Z_AXIS_MASK) << 4;
        b->job = STEPPER_HOME_REF;
    }
    b->base_ticks = 8000; // 2Kz ~ 25 mm/s

    STEPPER_QUEUE_push();

    p->position[X_AXIS] = p->target[X_AXIS];
    p->position[Y_AXIS] = p->target[Y_AXIS];
    p->position[Z_AXIS] = p->target[Z_AXIS];

}
