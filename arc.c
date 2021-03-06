
#include "platform.h"

#include "stepper.h"
#include "planner.h"
#include "lcd.h"



/******************************************
 * ARC Kernel for the planner
 * --------------------------
 * If you modify this, adopt the kernel in stepper.c as well !
 ******************************************/



/* Octants are defined as:
 *  Counter ClockWise (CCW)  ClockWise (CW)
 *     \     |     /          \     |     /
 *       \ 2 | 1 /              \13 | 14/
 *       3 \ | /  0           12  \ | / 15
 *   V-------+-------^      ^-------+-------V
 *       4 / | \  7           11  / | \ 8
 *       / 5 | 6 \              /10 | 9 \
 *     /     |     \          /     |     \
 */

typedef uint8_t octant_t;
#define FLAG_CW 8


// rendering offset to center, initially starting ccordinates
static int32_t x;
static int32_t y;

static octant_t octant; // bit 3 is set if CW , cleared for CCW

// rendering error: init to R**2 -X**2 -Y**2 -1
static int32_t err;
// increment of rendering error
static int32_t dx; // same as 2*x for speed optimisation
static int32_t dy; // same as 2*y for speed optimisation

static int32_t endx;
static int32_t endy;

// amount of points to trace
static uint32_t num_steps;


// return codes of kernel
#define ARC_KERNEL_NO_HIT 0
#define ARC_KERNEL_NO_MORE_STEPS 1
#define ARC_KERNEL_NEAR_HIT 2
#define ARC_KERNEL_HIT 3



octant_t get_octant(int32_t x, int32_t y, bool ccw) {
    if (ccw) { //CCW
        if ((x > 0) && (y >= 0)) {
            return (x > y) ? STEPPER_ARC_CCW_OCT1 : STEPPER_ARC_CCW_OCT2;
        } else if ((y > 0) && (x <= 0)) {
            return (y > -x) ? STEPPER_ARC_CCW_OCT3 : STEPPER_ARC_CCW_OCT4;
        } else if ((x<0) && (y <= 0)) {
            return (x < y) ? STEPPER_ARC_CCW_OCT5 : STEPPER_ARC_CCW_OCT6;
        } else if ((y < 0) && (x >= 0)) {
            return (y < -x) ? STEPPER_ARC_CCW_OCT7 : STEPPER_ARC_CCW_OCT8;
        }
    } else { //CW
        if ((x > 0) && (y >= 0)) {
            return (x > y) ? STEPPER_ARC_CW_OCT1 : STEPPER_ARC_CW_OCT2;
        } else if ((y > 0) && (x <= 0)) {
            return (y > -x) ? STEPPER_ARC_CW_OCT3 : STEPPER_ARC_CW_OCT4;
        } else if ((x<0) && (y <= 0)) {
            return (x < y) ? STEPPER_ARC_CW_OCT5 : STEPPER_ARC_CW_OCT6;
        } else if ((y < 0) && (x >= 0)) {
            return (y < -x) ? STEPPER_ARC_CW_OCT7 : STEPPER_ARC_CW_OCT8;
        }
    }
    return 0xff; // BAD VALUE
}



// returns ARC_KERNEL_NO_HIT until the final point is reached, or num_steps gets zero
// works directly on the static vars above!!! INIT correctly !!!
uint8_t kernel(void) {
    uint8_t flags = 0;

    switch (octant) {
    // CCW octants
    case STEPPER_ARC_CCW_OCT1:
            // octant 1: x > 0, y >= 0, x > y
            // always y+, sometimes x-
            if (err <= dy - x)
                flags |= KERNEL_FLAG_X_NEG;
            flags |= KERNEL_FLAG_Y_POS;

            if (y >= x)
                octant = STEPPER_ARC_CCW_OCT2;
            break;

    case STEPPER_ARC_CCW_OCT2:
            // octant 2: x > 0, y > 0, x <= y
            // always x-, sometimes y+
            if (err > y - dx)
                flags |= KERNEL_FLAG_Y_POS;
            flags |= KERNEL_FLAG_X_NEG;

            if (x <= 0)
                octant = STEPPER_ARC_CCW_OCT3;
            break;

    case STEPPER_ARC_CCW_OCT3:
            // octant 3: x <= 0, y > 0, -x <= y
            // always x-, sometimes y-
            if (err <= -(dx + y))
                flags |= KERNEL_FLAG_Y_NEG;
            flags |= KERNEL_FLAG_X_NEG;

            if (x <= -y)
                octant = STEPPER_ARC_CCW_OCT4;
            break;

    case STEPPER_ARC_CCW_OCT4:
            // octant 4: x < 0, y > 0, -x > y
            // always y-, sometimes x-
            if (err > -(dy + x))
                flags |= KERNEL_FLAG_X_NEG;
            flags |= KERNEL_FLAG_Y_NEG;

            if (y <= 0)
                octant = STEPPER_ARC_CCW_OCT5;
            break;

    case STEPPER_ARC_CCW_OCT5:
            // octant 5: x < 0, y <= 0, -x > -y
            // always y-, sometimes x+
            if (err <= x - dy)
                flags |= KERNEL_FLAG_X_POS;
            flags |= KERNEL_FLAG_Y_NEG;

            if (x >= y)
                octant = STEPPER_ARC_CCW_OCT6;
            break;

    case STEPPER_ARC_CCW_OCT6:
            // octant 6: x < 0, y < 0, -x <= -y
            // always x+, sometimes y-
            if (err > dx - y)
                flags |= KERNEL_FLAG_Y_NEG;
            flags |= KERNEL_FLAG_X_POS;

            if (x >= 0)
                octant = STEPPER_ARC_CCW_OCT7;
            break;

    case STEPPER_ARC_CCW_OCT7:
            // octant 7: x >= 0, y < 0, x <= -y
            // always x+, sometimes y+
            if (err <= dx + y)
                flags |= KERNEL_FLAG_Y_POS;
            flags |= KERNEL_FLAG_X_POS;

            if (x >= -y)
                octant = STEPPER_ARC_CCW_OCT8;
            break;

    case STEPPER_ARC_CCW_OCT8:
            // octant 8: x > 0, y < 0, x > -y
            // always y+, sometimes x+
            if (err > x + dy)
                flags |= KERNEL_FLAG_X_POS;
            flags |= KERNEL_FLAG_Y_POS;

            if (y >= 0)
                octant = STEPPER_ARC_CCW_OCT1;
            break;

    // CW octants
    case STEPPER_ARC_CW_OCT1:
            // octant 1: x > 0, y >= 0, x > y
            // always y-, sometimes x+
            if (err > -dy + x)
                flags |= KERNEL_FLAG_X_POS;
            flags |= KERNEL_FLAG_Y_NEG;

            if (y <= 0)
                octant = STEPPER_ARC_CW_OCT8;
            break;

    case STEPPER_ARC_CW_OCT2:
            // octant 2: x > 0, y > 0, x <= y
            // always x+, sometimes y-
            if (err <= -y + dx)
                flags |= KERNEL_FLAG_Y_NEG;
            flags |= KERNEL_FLAG_X_POS;

            if (y < x)
                octant = STEPPER_ARC_CW_OCT1;
            break;

    case STEPPER_ARC_CW_OCT3:
            // octant 3: x <= 0, y > 0, -x <= y
            // always x+, sometimes y+
            if (err > dx + y)
                flags |= KERNEL_FLAG_Y_POS;
            flags |= KERNEL_FLAG_X_POS;

            if (x >= 0)
                octant = STEPPER_ARC_CW_OCT2;
            break;

    case STEPPER_ARC_CW_OCT4:
            // octant 4: x < 0, y > 0, -x > y
            // always y+, sometimes x+
            if (err <= dy + x)
                flags |= KERNEL_FLAG_X_POS;
            flags |= KERNEL_FLAG_Y_POS;

            if (x > -y)
                octant = STEPPER_ARC_CW_OCT3;
            break;

    case STEPPER_ARC_CW_OCT5:
            // octant 5: x < 0, y <= 0, -x > -y
            // always y+, sometimes x-
            if (err > -x + dy)
                flags |= KERNEL_FLAG_X_NEG;
            flags |= KERNEL_FLAG_Y_POS;

            if (y >= 0)
                octant = STEPPER_ARC_CW_OCT4;
            break;

    case STEPPER_ARC_CW_OCT6:
            // octant 6: x < 0, y < 0, -x <= -y
            // always x-, sometimes y+
            if (err <= -dx + y)
                flags |= KERNEL_FLAG_Y_POS;
            flags |= KERNEL_FLAG_X_NEG;

            if (x < y)
                octant = STEPPER_ARC_CW_OCT5;
            break;

    case STEPPER_ARC_CW_OCT7:
            // octant 7: x >= 0, y < 0, x <= -y
            // always x-, sometimes y-
            if (err > -(dx + y))
                flags |= KERNEL_FLAG_Y_NEG;
            flags |= KERNEL_FLAG_X_NEG;

            if (x <= 0)
                octant = STEPPER_ARC_CW_OCT6;
            break;

    case STEPPER_ARC_CW_OCT8:
            // octant 8: x > 0, y < 0, x > -y
            // always y-, sometimes x-
            if (err <= -(x + dy))
                flags |= KERNEL_FLAG_X_NEG;
            flags |= KERNEL_FLAG_Y_NEG;

            if (x < -y)
                octant = STEPPER_ARC_CW_OCT7;
            break;
    };

    // dont reorder !!!
    if (flags & KERNEL_FLAG_X_NEG) {
        /* would STEP the stepper_x in negative direction */
        err += dx-1;
        dx -= 2;
        x--;
    }
    if (flags & KERNEL_FLAG_X_POS) {
        x++;
        dx += 2;
        err -= dx-1;
        /* would STEP the stepper_x in positive direction */
    }
    if (flags & KERNEL_FLAG_Y_NEG) {
        /* would STEP the stepper_y in negative direction */
        err += dy-1;
        dy -= 2;
        y--;
    }
    if (flags & KERNEL_FLAG_Y_POS) {
        y++;
        dy += 2;
        err -= dy-1;
        /* would STEP the stepper_y in positive direction */
    }


    /*
    LOG_STRING("arc:kernel\n octant = ");LOG_U8(octant);
    LOG_STRING("\n    err = ");LOG_S32(err);
    LOG_STRING("\n      x = ");LOG_S32(x);
    LOG_STRING("\n      y = ");LOG_S32(y);
    LOG_STRING("\n   endx = ");LOG_S32(endx);
    LOG_STRING("\n   endy = ");LOG_S32(endy);
    LOG_STRING("\n  steps = ");LOG_U32(num_steps);
    LOG_NEWLINE;
    */

    if (abs(x) > abs(y)) {
        // x is dominant, y MUST match
        if (endy == y) {
            LOG_STRING("Y:MATCH,");
            if (endx == x) {
                LOG_STRING("PERFECT X:MATCH\n");
                return ARC_KERNEL_HIT;
            } else if (sgn(x) == sgn(endx)) {
                LOG_STRING("almost X:MATCH\n");
                return ARC_KERNEL_NEAR_HIT;
            }
        }
    } else {
        // y is dominant, x MUST match
        if (endx == x) {
            LOG_STRING("X:MATCH,");
            if (endy == y) {
                LOG_STRING("PERFECT Y:MATCH\n");
                return ARC_KERNEL_HIT;
            } else if (sgn(y) == sgn(endy)) {
                LOG_STRING("almost Y:MATCH\n");
                return ARC_KERNEL_NEAR_HIT;
            }
        }
    }

    num_steps--;

    return (!num_steps) ? ARC_KERNEL_NO_MORE_STEPS : ARC_KERNEL_NO_HIT;
}

void init_arc(uint32_t R, StepperPos _x, StepperPos _y, bool ccw) {
    octant = get_octant(_x, _y, ccw);
    x = _x;
    y = _y;
    //~ err = ((int64_t) R) * ((int64_t) R) - ((int64_t) _x) * ((int64_t) _x) - ((int64_t) _y) * ((int64_t) _y) -1;
    dx = 2*x;
    dy = 2*y;
    num_steps = 6*R;
    //~ LOG_STRING("arc:init_arc("); LOG_U32(R);
    //~ LOG_COMMA; LOG_S24(_x);
    //~ LOG_COMMA; LOG_S24(_y);
    //~ LOG_COMMA; LOG_U8(ccw);
    //~ LOG_STRING(")\n octant = "); LOG_U32(octant);
    //~ LOG_STRING( "\n    err = "); LOG_S32(err);
    //~ LOG_STRING( "\n     dx = "); LOG_S32(dx);
    //~ LOG_STRING( "\n     dy = "); LOG_S32(dy);
    //~ LOG_STRING( "\n  steps = "); LOG_U32(num_steps);
    //~ LOG_NEWLINE;
}

void buffer_arc(uint32_t R, StepperPos fromx, StepperPos fromy,
                                   StepperPos tox, StepperPos toy, bool ccw) {
    block_t *b;
    uint8_t r;

    LOG_STRING("arc:buffer_arc(");
    LOG_U32(R);LOG_COMMA;
    LOG_S24(fromx);LOG_COMMA;
    LOG_S24(fromy);LOG_COMMA;
    LOG_S24(tox);LOG_COMMA;
    LOG_S24(toy);LOG_COMMA;
    LOG_U8(ccw);
    LOG_STRING(")\n");

    endx = tox;
    endy = toy;

    init_arc(R, fromx, fromy, ccw);
    // err will be zero at target point -> always exact match!
    err = ((uint64_t) tox) * ((uint64_t) tox) -
          ((uint64_t) fromx) * ((uint64_t) fromx) +
          ((uint64_t) toy) * ((uint64_t) toy) -
          ((uint64_t) fromy) * ((uint64_t) fromy);

    // iterate once through arc to count steps and lengths
    while (ARC_KERNEL_NO_HIT == (r = kernel())) ;

    // if no hit -> increase R (can not happen with new err-calculation
    //~ if (r == ARC_KERNEL_NO_MORE_STEPS) {
        //~ LOG_STRING("NO HIT: recurse!");LOG_NEWLINE;
        //~ return buffer_arc(R+1, fromx, fromy, tox, toy, ccw);
    //~ }

    // x and y are end position, update PLANNER.position to reflect the position after this arc
    PL.position[X_AXIS] = PL.arc_center_x + tox;
    PL.position[Y_AXIS] = PL.arc_center_y + toy;

    // send job to stepper queue
    b = &(STEPPER_QUEUE_data[STEPPER_QUEUE_head]);
    b->arc.stepcount = -num_steps;
    init_arc(R, fromx, fromy, ccw);
    // err will be zero at target point -> always exact match!
    err = ((uint64_t) tox) * ((uint64_t) tox) -
          ((uint64_t) fromx) * ((uint64_t) fromx) +
          ((uint64_t) toy) * ((uint64_t) toy) -
          ((uint64_t) fromy) * ((uint64_t) fromy);
    b->arc.stepcount += num_steps + 1;
    b->arc.x = fromx;
    b->arc.y = fromy;
    b->arc.err = err;
    b->arc.r = R;
    //~ b->base_ticks = 40404; // may be multiplied with 1..1.414 for OCR1A, depending on position on arc
    b->job = get_octant(fromx, fromy, ccw);

    // XXX: OPTIMIZE this! mul may overflow!
    b->laser.steps = (laser.pulses) ? laser.pulses : \
        ((uint64_t)b->arc.stepcount * (uint64_t)laser.ppm) / ((uint32_t) (XY_AXIS_STEPS_PER_M * 0.9003163161571061)); // ppm is a number: real value times 1000

    LOG_STRING("planner:arc_stepcount=");LOG_U32(b->arc.stepcount);LOG_NEWLINE;
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

    LOG_STRING("Final Position:");LOG_S24(PL.position[X_AXIS]);LOG_S24(PL.position[Y_AXIS]);LOG_NEWLINE;
}
