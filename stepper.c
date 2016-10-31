
#include "pins.h"
#include "hw_layout.h"
#include "platform.h"
#include "stepper.h"
#include "asm.h"

#ifdef USE_MCP4922
#include "mcp4922.h"
#endif

#define LOG_POS {if (laser.laser_is_on) {LOG_STRING("Stepper is at");LOG_S24(s->position[0]);LOG_COMMA;LOG_S24(s->position[1]);LOG_COMMA;LOG_S24(s->position[2]);LOG_NEWLINE;};}
#define LOG_STEP(dirstr) {LOG_STRING("Stepping " dirstr "\n");}

BUFFER(STEPPER_QUEUE, block_t, STEPPER_QUEUE_SIZE)

state_t STATE;

int32_t referenced_position[NUM_AXIS]; // position after referencing is done (before setting refpos)
int32_t lost_ref_steps[NUM_AXIS];       // difference between last referencing and current -> lost_steps

state_t *state_ptr;


/**********************************************************
 * Kernels move the laserhead                             *
 * -------------------------------------------------------*
 * kernels are handled first, tools later                 *
 * implemented kernels: move, arc, home, line             *
 **********************************************************/

/***************
 * MOVE KERNEL *
 ***************/

// ALWAYS supported. may use accel/deccel for faster moves. do only use with OFF tool (or none)
bool move_kernel(void) {
    register state_t *s asm("r28") = state_ptr;
    uint8_t flag;
    uint16_t tmp, maxcode;

    LOG_STRING("Stepper: MOVE\n");
    // first part: step the necessary axes
    if (s->axis_mask & X_AXIS_MASK) {
        if (s->count_direction[X_AXIS] == 1) {
            SET_X_INC;
        } else {
            SET_X_DEC;
        }
        ADVANCE_STEP_COUNTER(s->step_ctr[X_AXIS], s->steps[X_AXIS], s->steps_total, flag);
        if (flag) {
            SET_X_STEP;
            if (s->count_direction[X_AXIS] == 1) {LOG_STEP("X+");} else LOG_STEP("X-");
            ADVANCE_POSITION(s->position[X_AXIS], s->count_direction[X_AXIS]);
            CLR_X_STEP;
        }
    }
    if (s->axis_mask & Y_AXIS_MASK) {
        if (s->count_direction[Y_AXIS] == 1) {
            SET_Y_INC;
        } else {
            SET_Y_DEC;
        }
        ADVANCE_STEP_COUNTER(s->step_ctr[Y_AXIS], s->steps[Y_AXIS], s->steps_total, flag);
        if (flag) {
            SET_Y_STEP;
            if (s->count_direction[Y_AXIS] == 1) {LOG_STEP("Y+");} else LOG_STEP("Y-");
            ADVANCE_POSITION(s->position[Y_AXIS], s->count_direction[Y_AXIS]);
            CLR_Y_STEP;
        }
    }
    if (s->axis_mask & Z_AXIS_MASK) {
        if (s->count_direction[Z_AXIS] == 1) {
            SET_Z_INC;
        } else {
            SET_Z_DEC;
        }
        ADVANCE_STEP_COUNTER(s->step_ctr[Z_AXIS], s->steps[Z_AXIS], s->steps_total, flag);
        if (flag) {
            SET_Z_STEP;
            if (s->count_direction[Z_AXIS] == 1) {LOG_STEP("Z+");} else LOG_STEP("Z-");
            ADVANCE_POSITION(s->position[Z_AXIS], s->count_direction[Z_AXIS]);
            CLR_Z_STEP;
        }
    };
    LOG_POS;

    // fail-safe:
    if (!(s->axis_mask & 7))
        return FALSE; // axis mask empty -> finished

    // stupid ramp, but faster than staying at 244Hz. clamp to at most 8Khz.

    // 'mantissa' bits. the more, the slower is the accel/deccel. 6 is good
    #define MAGIC   6
    // highest code which fits into 16bits
    #define MAGIC2   (16-MAGIC+1)*(1<<MAGIC)

    // figure out highest code which (after decoding) is not bigger than s->base_ticks
    if (s->base_ticks < (1<<MAGIC)) {
        maxcode = s->base_ticks;
    } else {
        flag = 1;
        maxcode = s->base_ticks;
        while (maxcode >= (2<<MAGIC)) {
            flag++;
            maxcode >>= 1;
        }
        maxcode = (maxcode & ((1<<MAGIC)-1)) | (flag << MAGIC);
    }

    // obtain code to be used
    tmp = min(s->steps_to_go - 1, s->steps_total - s->steps_to_go);
    if (tmp <= maxcode)
        tmp = maxcode - tmp;
    else
        tmp = 0;

    // decode code to uint16
    // extract exponent (as flag)
    flag = tmp >> MAGIC;
    if (flag) {
        // if no underflow, force highest bit set
        tmp |= 1 << MAGIC;
        flag--;
    }
    // zero exponents bits
    tmp &= (2 << MAGIC) -1;
    // solve
    for(;flag;flag--) {
        if (tmp < 32768) {
            tmp <<= 1;
        } else {
            // overflow -> break loop
            tmp = 65535;
            break;
        }
    }
    OCR1A = max(2000U, min(s->base_ticks, tmp));
    LOG_STRING("ST: new OCR1A Value:");LOG_X16(OCR1A);LOG_NEWLINE;
    return (--s->steps_to_go != 0);
}

/**************
 * ARC KERNEL *
 **************/

#define USE_ARC_KERNEL
#ifdef USE_ARC_KERNEL

// ONLY to be called from ISR !!!
bool arc_kernel(void) {
    register state_t *s asm("r28") = state_ptr;
    register uint8_t flags=0;

    LOG_STRING("Stepper: ARC ");LOG_U8(s->job);LOG_NEWLINE;

    switch (s->job) {
    // CCW octants
    case STEPPER_ARC_CCW_OCT1:
            // octant 1: x > 0, y >= 0, x > y
            // always y+, sometimes x-
            if (s->arc_err <= s->arc_dy -s->arc_x) {
                flags |= KERNEL_FLAG_X_NEG;
                s->arc_x--;
            }
            s->arc_y++;
            flags |= KERNEL_FLAG_Y_POS;

            if (s->arc_y >= s->arc_x)
                s->job = STEPPER_ARC_CCW_OCT2;
            break;

    case STEPPER_ARC_CCW_OCT2:
            // octant 2: x > 0, y > 0, x <= y
            // always x-, sometimes y+
            if (s->arc_err > s->arc_y - s->arc_dx) {
                flags |= KERNEL_FLAG_Y_POS;
                s->arc_y++;
            }
            s->arc_x--;
            flags |= KERNEL_FLAG_X_NEG;

            if (s->arc_x <= 0)
                s->job = STEPPER_ARC_CCW_OCT3;
            break;

    case STEPPER_ARC_CCW_OCT3:
            // octant 3: x <= 0, y > 0, -x <= y
            // always x-, sometimes y-
            if (s->arc_err <= -(s->arc_dx + s->arc_y)) {
                flags |= KERNEL_FLAG_Y_NEG;
                s->arc_y--;
            }
            s->arc_x--;
            flags |= KERNEL_FLAG_X_NEG;

            if (s->arc_x <= -s->arc_y)
                s->job = STEPPER_ARC_CCW_OCT4;
            break;

    case STEPPER_ARC_CCW_OCT4:
            // octant 4: x < 0, y > 0, -x > y
            // always y-, sometimes x-
            if (s->arc_err > -(s->arc_dy + s->arc_x)) {
                flags |= KERNEL_FLAG_X_NEG;
                s->arc_x--;
            }
            s->arc_y--;
            flags |= KERNEL_FLAG_Y_NEG;

            if (s->arc_y <= 0)
                s->job = STEPPER_ARC_CCW_OCT5;
            break;

    case STEPPER_ARC_CCW_OCT5:
            // octant 5: x < 0, y <= 0, -x > -y
            // always y-, sometimes x+
            if (s->arc_err <= s->arc_x - s->arc_dy) {
                flags |= KERNEL_FLAG_X_POS;
                s->arc_x++;
            }
            s->arc_y--;
            flags |= KERNEL_FLAG_Y_NEG;

            if (s->arc_x >= s->arc_y)
                s->job = STEPPER_ARC_CCW_OCT6;
            break;

    case STEPPER_ARC_CCW_OCT6:
            // octant 6: x < 0, y < 0, -x <= -y
            // always x+, sometimes y-
            if (s->arc_err > s->arc_dx - s->arc_y) {
                flags |= KERNEL_FLAG_Y_NEG;
                s->arc_y--;
            }
            s->arc_x++;
            flags |= KERNEL_FLAG_X_POS;

            if (s->arc_x >= 0)
                s->job = STEPPER_ARC_CCW_OCT7;
            break;

    case STEPPER_ARC_CCW_OCT7:
            // octant 7: x >= 0, y < 0, x <= -y
            // always x+, sometimes y+
            if (s->arc_err <= s->arc_dx + s->arc_y) {
                flags |= KERNEL_FLAG_Y_POS;
                s->arc_y++;
            }
            s->arc_x++;
            flags |= KERNEL_FLAG_X_POS;

            if (s->arc_x >= -s->arc_y)
                s->job = STEPPER_ARC_CCW_OCT8;
            break;

    case STEPPER_ARC_CCW_OCT8:
            // octant 8: x > 0, y < 0, x > -y
            // always y+, sometimes x+
            if (s->arc_err > s->arc_x + s->arc_dy) {
                flags |= KERNEL_FLAG_X_POS;
                s->arc_x++;
            }
            s->arc_y++;
            flags |= KERNEL_FLAG_Y_POS;

            if (s->arc_y >= 0)
                s->job = STEPPER_ARC_CCW_OCT1;
            break;

    // CW octants
    case STEPPER_ARC_CW_OCT1:
            // octant 1: x > 0, y >= 0, x > y
            // always y-, sometimes x+
            if (s->arc_err > -s->arc_dy + s->arc_x) {
                flags |= KERNEL_FLAG_X_POS;
                s->arc_x++;
            }
            s->arc_y--;
            flags |= KERNEL_FLAG_Y_NEG;

            if (s->arc_y <= 0)
                s->job = STEPPER_ARC_CW_OCT8;
            break;

    case STEPPER_ARC_CW_OCT2:
            // octant 2: x > 0, y > 0, x <= y
            // always x+, sometimes y-
            if (s->arc_err <= -s->arc_y + s->arc_dx) {
                flags |= KERNEL_FLAG_Y_NEG;
                s->arc_y--;
            }
            s->arc_x++;
            flags |= KERNEL_FLAG_X_POS;

            if (s->arc_y < s->arc_x)
                s->job = STEPPER_ARC_CW_OCT1;
            break;

    case STEPPER_ARC_CW_OCT3:
            // octant 3: x <= 0, y > 0, -x <= y
            // always x+, sometimes y+
            if (s->arc_err > s->arc_dx + s->arc_y) {
                flags |= KERNEL_FLAG_Y_POS;
                s->arc_y++;
            }
            s->arc_x++;
            flags |= KERNEL_FLAG_X_POS;

            if (s->arc_x >= 0)
                s->job = STEPPER_ARC_CW_OCT2;
            break;

    case STEPPER_ARC_CW_OCT4:
            // octant 4: x < 0, y > 0, -x > y
            // always y+, sometimes x+
            if (s->arc_err <= s->arc_dy + s->arc_x) {
                flags |= KERNEL_FLAG_X_POS;
                s->arc_x++;
            }
            s->arc_y++;
            flags |= KERNEL_FLAG_Y_POS;

            if (s->arc_x > -s->arc_y)
                s->job = STEPPER_ARC_CW_OCT3;
            break;

    case STEPPER_ARC_CW_OCT5:
            // octant 5: x < 0, y <= 0, -x > -y
            // always y+, sometimes x-
            if (s->arc_err > -s->arc_x + s->arc_dy) {
                flags |= KERNEL_FLAG_X_NEG;
                s->arc_x--;
            }
            s->arc_y++;
            flags |= KERNEL_FLAG_Y_POS;

            if (s->arc_y >= 0)
                s->job = STEPPER_ARC_CW_OCT4;
            break;

    case STEPPER_ARC_CW_OCT6:
            // octant 6: x < 0, y < 0, -x <= -y
            // always x-, sometimes y+
            if (s->arc_err <= -s->arc_dx + s->arc_y) {
                flags |= KERNEL_FLAG_Y_POS;
                s->arc_y++;
            }
            s->arc_x--;
            flags |= KERNEL_FLAG_X_NEG;

            if (s->arc_x < s->arc_y)
                s->job = STEPPER_ARC_CW_OCT5;
            break;

    case STEPPER_ARC_CW_OCT7:
            // octant 7: x >= 0, y < 0, x <= -y
            // always x-, sometimes y-
            if (s->arc_err > -(s->arc_dx + s->arc_y)) {
                flags |= KERNEL_FLAG_Y_NEG;
                s->arc_y--;
            }
            s->arc_x--;
            flags |= KERNEL_FLAG_X_NEG;

            if (s->arc_x <= 0)
                s->job = STEPPER_ARC_CW_OCT6;
            break;

    case STEPPER_ARC_CW_OCT8:
            // octant 8: x > 0, y < 0, x > -y
            // always y-, sometimes x-
            if (s->arc_err <= -(s->arc_x + s->arc_dy)) {
                flags |= KERNEL_FLAG_X_NEG;
                s->arc_x--;
            }
            s->arc_y--;
            flags |= KERNEL_FLAG_Y_NEG;

            if (s->arc_x < -s->arc_y)
                s->job = STEPPER_ARC_CW_OCT7;
            break;
    };
    LOG_STRING("ARC:FLAGS");LOG_X8(flags);LOG_NEWLINE;
    // evaluate flags
    if (flags & KERNEL_FLAG_X_NEG) {
        /* set direction */
        SET_X_DEC;
        /* book-keeping */
        s->arc_err += s->arc_dx-1;
        s->arc_dx -= 2;
        //~ s->arc_x--;
        /* STEP the stepper_x in negative direction */
        SET_X_STEP;
        s->position[X_AXIS]--;
        LOG_STEP("X-");
        CLR_X_STEP;
    }
    if (flags & KERNEL_FLAG_X_POS) {
        /* set direction */
        SET_X_INC;
        /* book-keeping */
        //~ s->arc_x++;
        s->arc_dx += 2;
        s->arc_err -= s->arc_dx-1;
        /* STEP the stepper_x in positive direction */
        SET_X_STEP;
        s->position[X_AXIS]++;
        LOG_STEP("X+");
        CLR_X_STEP;
    }
    if (flags & KERNEL_FLAG_Y_NEG) {
        /* set direction */
        SET_Y_DEC;
        /* book-keeping */
        s->arc_err += s->arc_dy-1;
        s->arc_dy -= 2;
        //~ s->arc_y--;
        /* STEP the stepper_Y in negative direction */
        SET_Y_STEP;
        s->position[Y_AXIS]--;
        LOG_STEP("Y-");
        CLR_Y_STEP;
    }
    if (flags & KERNEL_FLAG_Y_POS) {
        /* set direction */
        SET_Y_INC;
        /* book-keeping */
        //~ s->arc_y++;
        s->arc_dy += 2;
        s->arc_err -= s->arc_dy-1;
        /* STEP the stepper_Y in positive direction */
        SET_Y_STEP;
        s->position[Y_AXIS]++;
        LOG_STEP("Y+");
        CLR_Y_STEP;
    }

    // handle Z movement, if any
    if (s->axis_mask & Z_AXIS_MASK) {
        uint8_t flag;
        if (s->count_direction[Z_AXIS] == 1) {
            SET_Z_INC;
        } else {
            SET_Z_DEC;
        }
        ADVANCE_STEP_COUNTER(s->step_ctr[Z_AXIS], s->steps[Z_AXIS], s->steps_total, flag);
        if (flag) {
            SET_Z_STEP;
            if (s->count_direction[Z_AXIS] == 1) {LOG_STEP("Z+");} else LOG_STEP("Z-");
            ADVANCE_POSITION(s->position[Z_AXIS], s->count_direction[Z_AXIS]);
            CLR_Z_STEP;
        }
    }

    LOG_POS;

    //~ avg_speed in x: |y|/R * mm_per_step * tickrate
    //~ total avg_speed is R/max(|x|,|y|) * mm_per_step * steprate
    //~ ->
    //~ F_CPU/OCR1A = steprate = avg_speed * max(|x|,|y|) / (R*mm_per_step)
    //~ ->
    //~ OCR1A = F_CPU * R * mm_per_step / (avg_speed * max(|x|,|y|)
    //~ at hor/vert tangents:
    //~ OCR1A_0 = F_CPU * mm_per_step / avg_speed = s-> base_ticks
    //~ ->
    //~ OCR1A = s->base_ticks * R / max(|x|,|y|)  = s->base_ticks ... sqrt(2)*s->base_ticks
    //~ -> s->base_ticks <= 40404 !!!!

    //s->speedfactor = round(256.0*sqrt(square(s->arc_x) + square(s->arc_y)) / max(abs(s->arc_x), abs(s->arc_y)));

    OCR1A = (((uint32_t) s->base_ticks) * (s->arc_radius)) / max(abs(s->arc_x),abs(s->arc_y));
    //~ OCR1A = s->base_ticks; // XXX: speedfactor correction!

    LOG_STRING("new OCR1A:");LOG_X16(OCR1A);LOG_X16(s->base_ticks);LOG_NEWLINE;

    s->steps_to_go--;
    return (s->steps_to_go != 0);
}

#endif



/***************
 * LINE KERNEL *
 ***************/

#ifdef USE_LINE_KERNEL
bool line_kernel(void) {
    register state_t *s asm("r28") = state_ptr;
    uint8_t flag;

    LOG_STRING("Stepper: LINE\n");

    // first part: step the necessary axes
    if (s->axis_mask & X_AXIS_MASK) {
        if (s->count_direction[X_AXIS] == 1) {
            SET_X_INC;
        } else {
            SET_X_DEC;
        }
        ADVANCE_STEP_COUNTER(s->step_ctr[X_AXIS], s->steps[X_AXIS], s->steps_total, flag);
        if (flag) {
            SET_X_STEP;
            if (s->count_direction[X_AXIS] == 1) {LOG_STEP("X+");} else LOG_STEP("X-");
            ADVANCE_POSITION(s->position[X_AXIS], s->count_direction[X_AXIS]);
            CLR_X_STEP;
        }
    }
    if (s->axis_mask & Y_AXIS_MASK) {
        if (s->count_direction[Y_AXIS] == 1) {
            SET_Y_INC;
        } else {
            SET_Y_DEC;
        }
        ADVANCE_STEP_COUNTER(s->step_ctr[Y_AXIS], s->steps[Y_AXIS], s->steps_total, flag);
        if (flag) {
            SET_Y_STEP;
            if (s->count_direction[Y_AXIS] == 1) {LOG_STEP("Y+");} else LOG_STEP("Y-");
            ADVANCE_POSITION(s->position[Y_AXIS], s->count_direction[Y_AXIS]);
            CLR_Y_STEP;
        }
    }
    if (s->axis_mask & Z_AXIS_MASK) {
        if (s->count_direction[Z_AXIS] == 1) {
            SET_Z_INC;
        } else {
            SET_Z_DEC;
        }
        ADVANCE_STEP_COUNTER(s->step_ctr[Z_AXIS], s->steps[Z_AXIS], s->steps_total, flag);
        if (flag) {
            SET_Z_STEP;
            if (s->count_direction[Z_AXIS] == 1) {LOG_STEP("Z+");} else LOG_STEP("Z-");
            ADVANCE_POSITION(s->position[Z_AXIS], s->count_direction[Z_AXIS]);
            CLR_Z_STEP;
        }
    };
    LOG_POS;

    OCR1A = s->base_ticks;

    // fail-safe:
    if (!(s->axis_mask & 7))
        return FALSE; // axis mask empty -> finished

    return (--s->steps_to_go != 0);
}
#endif



/***************
 * HOME KERNEL *
 ***************/

#ifdef USE_HOME_KERNEL

bool home_kernel(void) {
    register state_t *s asm("r28") = state_ptr;

    LOG_STRING("Stepper: HOME"); LOG_U8(s->job);LOG_NEWLINE;

    if(SIM_ACTIVE)
       s->job = STEPPER_HOME_REF;

    if (s->job == STEPPER_HOME) {
        // first part of homing: find the ref switches
        // check limit switches
        #ifdef X_HOME_IS_ACTIVE
        if ((s->axis_mask & X_AXIS_MASK) && (X_HOME_IS_ACTIVE))
        #endif
            s->axis_mask &=~ X_AXIS_MASK;

        #ifdef Y_HOME_IS_ACTIVE
        if ((s->axis_mask & Y_AXIS_MASK) && (Y_HOME_IS_ACTIVE))
        #endif
            s->axis_mask &=~ Y_AXIS_MASK;

        #ifdef Z_HOME_IS_ACTIVE
        if ((s->axis_mask & Z_AXIS_MASK) && (Z_HOME_IS_ACTIVE))
        #endif
            s->axis_mask &=~ Z_AXIS_MASK;

        // step the steppers
        if (s->axis_mask & X_AXIS_MASK) {
            SET_X_STEP;
            ADVANCE_POSITION(s->position[X_AXIS], s->count_direction[X_AXIS]);
            CLR_X_STEP;
        }

        if (s->axis_mask & Y_AXIS_MASK) {
            SET_Y_STEP;
            ADVANCE_POSITION(s->position[Y_AXIS], s->count_direction[Y_AXIS]);
            CLR_Y_STEP;
        }

        if (s->axis_mask & Z_AXIS_MASK) {
            SET_Z_STEP;
            ADVANCE_POSITION(s->position[Z_AXIS], s->count_direction[Z_AXIS]);
            CLR_Z_STEP;

        }

        LOG_POS;
        if (s->axis_mask & 0x07) // not yet finished, continue in next IRQ
            return TRUE;

        // re-init for second part
        s->axis_mask = (s->axis_mask >> 4) * 0x11; // upper 4 bits stored a copy of the original mask

        s->count_direction[0] = -X_HOME_DIR;
        s->count_direction[1] = -Y_HOME_DIR;
        s->count_direction[2] = -Z_HOME_DIR;
        if (s->count_direction[X_AXIS] == 1) SET_X_INC; else SET_X_DEC;
        if (s->count_direction[Y_AXIS] == 1) SET_Y_INC; else SET_Y_DEC;
        if (s->count_direction[Z_AXIS] == 1) SET_Z_INC; else SET_Z_DEC;
        s->job = STEPPER_HOME_REF;  // search for refpoint
        OCR1A = 65535; // second part is as slow as possible
        return TRUE;

    } else {
        // 2(nd) part: find refpos

        // check limit switches
        #ifdef X_HOME_IS_ACTIVE
        if ((s->axis_mask & X_AXIS_MASK) && (!X_HOME_IS_ACTIVE))
        #endif
                s->axis_mask &=~ X_AXIS_MASK;

        #ifdef Y_HOME_IS_ACTIVE
        if ((s->axis_mask & Y_AXIS_MASK) && (!Y_HOME_IS_ACTIVE))
        #endif
                s->axis_mask &=~ Y_AXIS_MASK;

        #ifdef Z_HOME_IS_ACTIVE
        if ((s->axis_mask & Z_AXIS_MASK) && (!Z_HOME_IS_ACTIVE))
        #endif
                s->axis_mask &=~ Z_AXIS_MASK;

        // step the steppers
        if (s->axis_mask & X_AXIS_MASK) {
            SET_X_STEP;
            ADVANCE_POSITION(s->position[X_AXIS], s->count_direction[X_AXIS]);
            CLR_X_STEP;
        }

        if (s->axis_mask & Y_AXIS_MASK) {
            SET_Y_STEP;
            ADVANCE_POSITION(s->position[Y_AXIS], s->count_direction[Y_AXIS]);
            CLR_Y_STEP;
        }

        if (s->axis_mask & Z_AXIS_MASK) {
            SET_Z_STEP;
            ADVANCE_POSITION(s->position[Z_AXIS], s->count_direction[Z_AXIS]);
            CLR_Z_STEP;
        }

        LOG_POS;
        if (SIM_ACTIVE) s->axis_mask &= 0xf0;
        if (s->axis_mask & 0x07)
            return TRUE; // not yet finished, continue in next IRQ
        // all done:
        // we now record the amount of 'lost steps' and the current position

        s->axis_mask = (s->axis_mask >> 4); // restore axis-mask

        if (s->axis_mask & X_AXIS_MASK) {
            referenced_position[X_AXIS] = s->position[X_AXIS];
            s->position[X_AXIS] = s->refpos[X_AXIS];
            lost_ref_steps[X_AXIS] = s->position[X_AXIS] - referenced_position[X_AXIS];
            LOG_STRING("ST: Setting Refpos X:");LOG_S24(s->position[X_AXIS]);LOG_NEWLINE;
        }
        if (s->axis_mask & Y_AXIS_MASK) {
            referenced_position[Y_AXIS] = s->position[Y_AXIS];
            s->position[Y_AXIS] = s->refpos[Y_AXIS];
            lost_ref_steps[Y_AXIS] = s->position[Y_AXIS] - referenced_position[Y_AXIS];
            LOG_STRING("ST: Setting Refpos Y:");LOG_S24(s->position[Y_AXIS]);LOG_NEWLINE;
        }
        if (s->axis_mask & Z_AXIS_MASK) {
            referenced_position[Z_AXIS] = s->position[Z_AXIS];
            s->position[Z_AXIS] = s->refpos[Z_AXIS];
            lost_ref_steps[Z_AXIS] = s->position[Z_AXIS] - referenced_position[Z_AXIS];
            LOG_STRING("ST: Setting Refpos Z:");LOG_S24(s->position[Z_AXIS]);LOG_NEWLINE;
        }
        s->job = STEPPER_IDLE;
        OCR1A = 65535;
        LOG_POS;
    }
    return FALSE; // indicate end of job
}
#endif



/********************
 * Main Stepper IRQ *
 ********************/
#ifdef DEBUG
static uint16_t laser_pulses;
static uint16_t bytes_fetched;
#endif

//~ static dbgctr=0;
inline void _stepper_irq(void) {
//ISR(TIMER1_COMPA_vect) {
    register state_t *s asm("r28") = state_ptr;
    block_t *b = &STEPPER_QUEUE_data[STEPPER_QUEUE_tail];

    CHECK_STACK;
    /*******************
     * 1) handle Laser *
     *******************/

    if (s->laser.last_mode != s->laser.mode) {
        if ((s->laser.last_mode & LASER_CW) && (s->laser.mode & LASER_PULSE)) {
            // switch off CW laser before first pulse
            laser_fire(0);
        }
        if ((s->laser.last_mode & LASER_PULSE) && (s->laser.mode & LASER_CW)) {
            // switch off PULSE before setting CW
            laser_start_pulse_timer(0);
        }
        if (s->laser.mode == LASER_OFF) {
            laser_fire(0);
#ifdef DEBUG
            LOG_STRING("Clearing dbg_counters\n");
            laser_pulses=bytes_fetched=0;
#endif
        }
        s->laser.last_mode = s->laser.mode;
    }

    LOG_STRING("ST: axis_mask:");LOG_U8(s->axis_mask);LOG_NEWLINE;
    // handle laser_fire
    if (s->axis_mask & LASER_MASK) {
        uint8_t flag;
        LOG_STRING("ST: check_laser: ctr/steps/steps_total:");LOG_S24(s->laser.ctr);LOG_S24(s->laser.steps);LOG_S24(s->steps_total);LOG_NEWLINE;
        ADVANCE_STEP_COUNTER(s->laser.ctr, s->laser.steps, s->steps_total, flag);
        if (flag) {
            uint8_t modulate_value = 0xff;
            // update out setting, check mode and modulation
            // check modulation first:
            LOG_STRING("ST: laser: mode/modulation:");LOG_U8(s->laser.mode);LOG_U8(s->laser.modulation+1);LOG_NEWLINE;
            if (s->laser.mode & LASER_MODULATE) {
                // refill restbits if needed. (not enough bits in modbits)
                if (s->laser.valid_bits <= s->laser.modulation) {
                    if (LASER_RASTERDATA_can_read()) {
                        LOG_STRING("ST: Fetch now modulation data:");
                        s->laser.modbits = (s->laser.modbits << s->laser.valid_bits) | LASER_RASTERDATA_get();
                        LOG_X8(s->laser.modbits);LOG_NEWLINE;
#ifdef DEBUG
                        LOG_STRING("Fetched");LOG_U16(++bytes_fetched);LOG_STRING("Bytes this line\n");
#endif
                    } else {
                        LOG_STRING("ST: Modulation data underflow\n");
                        s->laser.modbits <<= s->laser.valid_bits; // BUFFER UNDERFLOW !, fill with 0
                    }
                    s->laser.valid_bits += 8;
                }

                // transfer 'enough' bits into high byte of modbits
                s->laser.modbits <<= 1 + s->laser.modulation - max((int8_t)0, (int8_t)s->laser.valid_bits - 8);
                s->laser.valid_bits -= 1 + s->laser.modulation;

                // move 'modulation+1'bits from the (LSB-aligned) high byte of modbits into modulate_value
                modulate_value = s->laser.modbits >> 8;
                s->laser.modbits &= 0xff; // XXX: maybe only stor 8 bits in s->laser.modbits?

                // now we have to generate an 8-bit value from LSB-aligned modulation+1 bits
                switch (s->laser.modulation) {
                    case LASER_MODULATION_1BIT:
                        // map bit 0 to 0x00 and bit 1 to 0xff
                        modulate_value = (modulate_value) ? 0xff : 0;
                        break;
                    case LASER_MODULATION_2BIT:
                        modulate_value = modulate_value * 0b01010101;
                        break;
                    case LASER_MODULATION_3BIT:
                        // replicate to 6 bits and handle like 6 bits value
                        modulate_value = modulate_value * 0b00001001;
                        // intentionally no break !
                    case LASER_MODULATION_6BIT:
                        // shift left by two and copy top 2 MSBits into LSBits
                        modulate_value = (modulate_value << 2) | (modulate_value >> 4);
                        break;
                    case LASER_MODULATION_4BIT:
                        // replicate 2 nibbles
                        modulate_value = modulate_value * 0b00010001;
                        break;
                    case LASER_MODULATION_8BIT:
                        // nothing to do, already 8 bits are used....
                        break;
                }
                LOG_STRING("DBG: cooked modulate value");LOG_U8(modulate_value);LOG_NEWLINE;
#ifdef DEBUG
                LOG_STRING("Laser pulses this line:");LOG_U16(++laser_pulses);LOG_NEWLINE;
#endif

                LOG_STRING("ST: Laser: valid_bits/modbits/modulate_value:");LOG_U8(s->laser.valid_bits);LOG_X16(s->laser.modbits);LOG_X8(modulate_value);LOG_NEWLINE;
                // modulate value is now recent. check mode to see what should be modulated.
                if (s->laser.mode & LASER_CW) {
                    // modulate power
                    LOG_STRING("ST: LASER_RASTER_CW ");LOG_U8(modulate_value);LOG_NEWLINE;
                    laser_fire(modulate_value);
                }
                if (s->laser.mode & LASER_PULSE) {
                    // modulate pulse
                    if (modulate_value) {
                        LOG_STRING("ST: LASER_RASTER_PULSE");LOG_U32((s->laser.pulse_duration_us * modulate_value) >> 8);LOG_STRING("us\n");
                        laser_fire(255);
                        laser_start_pulse_timer((s->laser.pulse_duration_us * (uint32_t)modulate_value) >> 8);
                    }
                }
            } else {
                // no modulation but maybe we need to PULSE
                if (s->laser.mode & LASER_PULSE) {
                    // modulate pulse
                    LOG_STRING("ST: LASER_PULSE");LOG_U32(s->laser.pulse_duration_us);LOG_STRING("us\n");
                    laser_fire(255); // !!!
                    laser_start_pulse_timer(s->laser.pulse_duration_us);
                }
                if (s->laser.mode & LASER_CW) {
                    // switch laser on
                    LOG_STRING("ST: LASER_CW");LOG_U8(modulate_value);LOG_NEWLINE;
                    laser_fire(255); // !!!
                    laser_start_pulse_timer(4000000); // 4s max
                }
                if (s->laser.mode == LASER_OFF) {
                    laser_fire(0);
                }
            }
        }
    }


    /*************************
     * 2) handle XYZ movement *
     *************************/
    // evaluate the current job and act accordingly

    // do some work. break if job completed, else return!
    LOG_STRING("Stepper: Checking current job: ");
    LOG_U8(s->job);
    switch(s->job) {
#ifdef USE_ARC_KERNEL
        case STEPPER_ARC_CCW_OCT1 ... STEPPER_ARC_CW_OCT8:
            LOG_STRING("ARC\n");
            if (arc_kernel())
                return;
            LOG_STRING("ARC finished\n");
            break; // end of arc -> break -> fetch next job
#endif

        case STEPPER_LINE:
            LOG_STRING("LINE\n");
#ifdef USE_LINE_KERNEL
            if (line_kernel())
                return;
#else
            if (move_kernel())
                return;
#endif
            LOG_STRING("LINE finished\n");
            break;

        case STEPPER_MOVE_TO:
            LOG_STRING("MOVE\n");
            if (move_kernel())
                return;
            LOG_STRING("MOVE finished\n");
            break;

        case STEPPER_HOME:
        case STEPPER_HOME_REF:
            LOG_STRING("HOME(REF)\n");
#ifdef USE_HOME_KERNEL
            if (home_kernel())
                return;
            LOG_STRING("HOME finished\n");
#endif
            break;

        case STEPPER_IDLE:
            LOG_STRING("IDLE\n");
            // nothing is done here.... see below
            break;
    }

    // break was called, or unknown job
    s->job = STEPPER_IDLE;
    s->laser.mode = LASER_OFF;

#ifdef DEBUG
            LOG_STRING("Clearing dbg_counters\n");
            laser_pulses=bytes_fetched=0;
#endif

    if (!STEPPER_QUEUE_is_empty()) {
        LOG_STRING("Stepper: fetch next job: ");
        // try to fetch next job
        b = &STEPPER_QUEUE_data[STEPPER_QUEUE_tail]; // update our job-pointer

        // clean restbits
        s->laser.valid_bits = 0;

        // store copy of current mode for detection at first irq of next job
        s->laser.last_mode = LASER_OFF;

        // update our Laser options (hard disable laser as default)
        s->laser.ctr = 0;
        s->laser.steps = 0;
        s->laser.pulse_duration_us = 0;
        s->laser.power = 0;
        s->laser.mode = 0;
        s->laser.modulation = 0;

        // copy job data to state
        s->job = b->job;

        // init book-keeping depending on job
        LOG_U8(s->job);
        switch (s->job) {

            case STEPPER_ARC_CCW_OCT1 ... STEPPER_ARC_CW_OCT8:
                LOG_STRING("ARC\n");
                s->steps_to_go = s->steps_total = b->arc.stepcount;
                s->arc_x = b->arc.x;
                s->arc_dx = 2* s->arc_x;
                s->arc_y = b->arc.y;
                s->arc_dy = 2* s->arc_y;
                s->arc_err = b->arc.err;
                s->arc_radius = b->arc.r;
                s->base_ticks = b->base_ticks;
                s->axis_mask = X_AXIS_MASK | Y_AXIS_MASK; // no Z atm. !!!
                // update our Laser options
                if (b->laser.mode != LASER_OFF) {
                    s->axis_mask |= LASER_MASK;
                    s->laser.ctr = ((s->laser.mode & LASER_RASTER_CW) != 0) ? (-1) : (-((int24_t)(s->steps_total/2)));
                    s->laser.steps = b->laser.steps;
                    s->laser.pulse_duration_us = b->laser.pulse_duration_us;
                    s->laser.power = b->laser.power;
                    s->laser.mode = b->laser.mode;
                    s->laser.modulation = b->laser.modulation;
                }
                break;

            case STEPPER_MOVE_TO:
                LOG_STRING("MOVE\n");
                s->axis_mask = 0;
                // copy number of steps per direction
                s->steps[0] = b->move.steps_x;
                if (s->steps[0]) {
                    s->axis_mask |= X_AXIS_MASK;
                    LOG_CHAR('X');LOG_U24(s->steps[0]);LOG_NEWLINE;
                }
                s->steps[1] = b->move.steps_y;
                if (s->steps[1]) {
                    s->axis_mask |= Y_AXIS_MASK;
                    LOG_CHAR('Y');LOG_U24(s->steps[1]);LOG_NEWLINE;
                }
                s->steps[2] = b->move.steps_z;
                if (s->steps[2]) {
                    s->axis_mask |= Z_AXIS_MASK;
                    LOG_CHAR('Z');LOG_U24(s->steps[2]);LOG_NEWLINE;
                }
                s->steps_to_go = s->steps_total = max(b->move.steps_x, max(b->move.steps_y, b->move.steps_z));
                s->step_ctr[2] = s->step_ctr[1] = s->step_ctr[0] = -(s->steps_total/2);

                LOG_STRING("ST: total_Steps:");LOG_U24(s->steps_total);LOG_NEWLINE;
                LOG_STRING("ST: initial error counter:");LOG_X24(s->step_ctr[2]);LOG_NEWLINE;

                s->count_direction[0] = (b->move.direction_bits & X_AXIS_MASK) ? -1 : +1;
                s->count_direction[1] = (b->move.direction_bits & Y_AXIS_MASK) ? -1 : +1;
                s->count_direction[2] = (b->move.direction_bits & Z_AXIS_MASK) ? -1 : +1;
                s->base_ticks = b->base_ticks;
                break;

            case STEPPER_LINE:
                LOG_STRING("LINE\n");
                s->axis_mask = 0;
                // copy number of steps per direction
                s->steps[0] = b->move.steps_x;
                if (s->steps[0]) {
                    s->axis_mask |= X_AXIS_MASK;
                    LOG_CHAR('X');LOG_U24(s->steps[0]);LOG_NEWLINE;
                }
                s->steps[1] = b->move.steps_y;
                if (s->steps[1]) {
                    s->axis_mask |= Y_AXIS_MASK;
                    LOG_CHAR('Y');LOG_U24(s->steps[1]);LOG_NEWLINE;
                }
                s->steps[2] = b->move.steps_z;
                if (s->steps[2]) {
                    s->axis_mask |= Z_AXIS_MASK;
                    LOG_CHAR('Z');LOG_U24(s->steps[2]);LOG_NEWLINE;
                }
                s->steps_to_go = s->steps_total = max(b->move.steps_x, max(b->move.steps_y, b->move.steps_z));
                LOG_U24(s->steps_total);LOG_STRING("Total steps\n");

                s->step_ctr[2] = s->step_ctr[1] = s->step_ctr[0] = -(s->steps_total/2);
                s->count_direction[0] = (b->line.direction_bits & X_AXIS_MASK) ? -1 : +1;
                s->count_direction[1] = (b->line.direction_bits & Y_AXIS_MASK) ? -1 : +1;
                s->count_direction[2] = (b->line.direction_bits & Z_AXIS_MASK) ? -1 : +1;
                s->base_ticks = b->base_ticks;

                // update our Laser options
                if (b->laser.mode != LASER_OFF) {
                    s->axis_mask |= LASER_MASK;
                    s->laser.ctr = ((s->laser.mode & LASER_RASTER_CW) != 0) ? (-1) : (-((int24_t)(s->steps_total/2)));
                    s->laser.steps = b->laser.steps;
                    s->laser.pulse_duration_us = b->laser.pulse_duration_us;
                    s->laser.power = b->laser.power;
                    s->laser.mode = b->laser.mode;
                    s->laser.modulation = b->laser.modulation;
                }
                break;

            case STEPPER_HOME:
                LOG_STRING("HOME\n");
                s->axis_mask = 0x77 & (b->ref.axis_mask * 0x11); // copy lowermost 3 bits to both nibbles
                s->count_direction[X_AXIS] = X_HOME_DIR;
                s->count_direction[Y_AXIS] = Y_HOME_DIR;
                s->count_direction[Z_AXIS] = Z_HOME_DIR;
                s->refpos[X_AXIS] = b->ref.pos[X_AXIS];
                s->refpos[Y_AXIS] = b->ref.pos[Y_AXIS];
                s->refpos[Z_AXIS] = b->ref.pos[Z_AXIS];
                s->base_ticks = b->base_ticks;

                LOG_STRING("ST: HOME: REFPOS IS:");LOG_S24(s->refpos[X_AXIS]);LOG_S24(s->refpos[Y_AXIS]);LOG_S24(s->refpos[Z_AXIS]);LOG_NEWLINE;
                break;

            case STEPPER_HOME_REF:  // used in set current position
                LOG_STRING("HOME_REF\n");
                s->axis_mask = b->ref.axis_mask;
                s->refpos[X_AXIS] = b->ref.pos[X_AXIS];
                s->refpos[Y_AXIS] = b->ref.pos[Y_AXIS];
                s->refpos[Z_AXIS] = b->ref.pos[Z_AXIS];
                s->base_ticks = b->base_ticks;

                LOG_STRING("ST: HOME_REF: REFPOS IS:");LOG_S24(s->refpos[X_AXIS]);LOG_S24(s->refpos[Y_AXIS]);LOG_S24(s->refpos[Z_AXIS]);LOG_NEWLINE;
                break;

            case STEPPER_IDLE:
                LOG_STRING("IDLE\n");
                s->base_ticks = 65535;  // 3ms idle ticking
                break;

            default:
                LOG_STRING("UNHANDLED !!!\n");
        }
        STEPPER_QUEUE_pop(); // advance _tail to next job
        // init direction pins for next round
        if (s->count_direction[X_AXIS] == 1) SET_X_INC; else SET_X_DEC;
        if (s->count_direction[Y_AXIS] == 1) SET_Y_INC; else SET_Y_DEC;
        if (s->count_direction[Z_AXIS] == 1) SET_Z_INC; else SET_Z_DEC;

        if (s->laser.mode & LASER_PULSE)
            s->laser.ctr = -(s->steps_total/2);
        else
            s->laser.ctr = -1;


        LOG_STRING("New Job: properties:\n");
        LOG_STRING("Laser:  steps:");LOG_U24(s->laser.steps);LOG_NEWLINE;
        LOG_STRING("Laser:    ctr:");LOG_S24(s->laser.ctr);LOG_NEWLINE;
        LOG_STRING("Laser:  power:");LOG_U8(s->laser.power);LOG_NEWLINE;
        LOG_STRING("Laser:   mode:");LOG_U8(s->laser.mode);LOG_NEWLINE;
        LOG_STRING("Laser:   bits:");LOG_U8(s->laser.modulation+1);LOG_NEWLINE;
        LOG_STRING("Stepper:ticks:");LOG_X16(s->base_ticks);LOG_NEWLINE;

#ifdef USE_MCP4922
        mcp4922_set_master(s->laser.power<<2); // !!! 0..1000 -> 0..4095 ???
#else
        if (s->laser.mode == LASER_OFF)
            laser_fire(0);
#endif
        OCR1A = s->base_ticks;
    }
    LOG_STRING("OCR1A is:");LOG_X16(OCR1A);LOG_NEWLINE;
}
ISR(TIMER1_COMPA_vect) {
    ACTIVE_IRQ_2;
    _stepper_irq();
    ACTIVE_IRQ_NONE;
}


// waits until all jobs are completed!
void stepper_drain_buffer(void) {
    while (!STEPPER_QUEUE_is_empty())
        idle('S');
    while (STATE.job != STEPPER_IDLE)
        idle('s');
    // re-init RasterData Buffer as well
    LASER_RASTERDATA_init();
}

// read one component of the stepper position (in steps)
int32_t stepper_get_position(uint8_t idx) {
    int32_t value;
    CRITICAL_SECTION_START;
    value = STATE.position[idx];
    CRITICAL_SECTION_END;
    return value;
}

void stepper_init(void) {
    STEPPER_QUEUE_init();

    //Initialize stepper pins
    INIT_X;
    INIT_Y;
    INIT_Z;
    ENABLE_X;
    ENABLE_Y;
    ENABLE_Z;
    LASER_RASTERDATA_init();

    state_ptr = &STATE;
    // waveform generation = 0100 = CTC, output mode = 00 (disconnected), CS=010 (div by 8)
    TCCR1A = 0b00000000; // COM1A1,COM1A0,COM1B1,COM1B0,COM1C1,COM1C0,WGM11,WGM10
    // CS = 1 -> min_rate=244Hz
    TCCR1B = 0b00001001;// ICNC1,ICES1,-,WGM13,WGM12,CS12,CS11,CS10
    // CS = off, 1, 8, 64, 256, 1024, TC0, TC1

    OCR1A = 65535; // default 3ms wait
    TCNT1 = 0;
    TIMSK1 |= (1<<OCIE1A);

    LOG_STRING("ST: INIT finished");
};
