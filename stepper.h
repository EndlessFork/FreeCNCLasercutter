
#ifndef _STEPPER_H_
#define _STEPPER_H_

#include "platform.h"
#include "buffer.h"
#include "laser.h"

typedef enum {
    STEPPER_IDLE = 0,
    STEPPER_HOME,           // finding refswitches
    STEPPER_HOME_REF,       // finding refpos and setting new position
    STEPPER_MOVE_TO,        // NEVER FIRES LASER!
    STEPPER_LINE,           // may modulate the laser with rasterdata

    STEPPER_ARC_CCW = 16,
    STEPPER_ARC_CCW_OCT0 = 16, // 16..23: 8 octants for CCW arcs
    STEPPER_ARC_CCW_OCT1 = 17,
    STEPPER_ARC_CCW_OCT2 = 18,
    STEPPER_ARC_CCW_OCT3 = 19,
    STEPPER_ARC_CCW_OCT4 = 20,
    STEPPER_ARC_CCW_OCT5 = 21,
    STEPPER_ARC_CCW_OCT6 = 22,
    STEPPER_ARC_CCW_OCT7 = 23,

    STEPPER_ARC_CW = 24,
    STEPPER_ARC_CW_OCT0 = 24, // 24..31: 8 octants for CW arcs
    STEPPER_ARC_CW_OCT1 = 25,
    STEPPER_ARC_CW_OCT2 = 26,
    STEPPER_ARC_CW_OCT3 = 27,
    STEPPER_ARC_CW_OCT4 = 28,
    STEPPER_ARC_CW_OCT5 = 29,
    STEPPER_ARC_CW_OCT6 = 30,
    STEPPER_ARC_CW_OCT7 = 31

} stepper_job_t;

typedef struct {
    stepper_job_t job;
    union {
        // STEPPER_MOVE_TO: just move to new position
        struct {
            StepperCnt steps_x, steps_y, steps_z;
            mask_t direction_bits;
            // XXX: incude accel/deccel/vmax params
        } move; // 10 Bytes
        // STEPPER_HOME(_REF): referencing stuff
        struct {
            StepperPos pos[3];  // refpos
            mask_t  axis_mask; // which positions to home? (if none, just update position)
            // XXX: incude speed param
        } ref; // 10 Bytes
        // STEPPER_LINE: Lase in a straight line
        struct {
            StepperCnt steps[3]; // xyz
            mask_t direction_bits;
            // XXX: incude speed param
        } line; // 10 Bytes
        // STEPPER_ARC_...: Lase in an arc
        struct {
            StepperCnt stepcount;
            StepperPos x, y;
            int32_t err;
            StepperCnt r;
            // XXX: allow stepping of Z?
        } arc; // 15 bytes
    };
    // may be multiplied with 1..1.414 for arc's, else use this directly
    uint16_t base_ticks;
    // Laser opts. only used for LINE and ARC
    struct {
        StepperCnt   steps;
        uint24_t     pulse_duration_us; // pulse duration in us
        uint16_t     power:10; // 0..100% in units of 0.1 -> range is 0..1000;
        uint8_t      mode:3;
        uint8_t      modulation:3; // 0=BW, 1=2bits, 3=4Bits, 7=8Bits
    } laser; // 8 bytes
} block_t; // size=24 Bytes



typedef struct {
    stepper_job_t job;
    int8_t      count_direction[3];
    StepperPos  position[3];  // current position in steps
    mask_t      axis_mask;
    // kernel specific data
    union {
        StepperCnt  step_ctr[3]; // bresenham error counters   (XYZ)
        struct {
            StepperPos arc_x;
            StepperPos arc_y;
        };
    };
    union {
        StepperCnt  steps[3]; // current 'steps-to-take' (bresenham lengths) (XYZ)
        struct {
            StepperPos arc_dx;
            StepperPos arc_dy;
        };
        struct {
            StepperPos refpos[3];
        };
    };
    StepperCnt  steps_total; // total steps to take for the current job
    StepperCnt  steps_to_go;  // steps to do for current job. starts at steps_total. If == 0 -> job done
    int32_t     arc_err;
    StepperCnt  arc_radius;
    uint16_t    base_ticks;
    struct {
        StepperCnt  ctr;
        StepperCnt  steps;
        uint24_t    pulse_duration_us; // pulse duration in us
        uint16_t    power:10; // 0..100% in units of 0.1 -> range is 0..1000;
        uint8_t     mode:3;
        uint8_t     last_mode:3; // mode of previous job
        uint8_t     modulation:3; // modulation bits: 1/2/4/8  -1(may support others as well)
        uint8_t     valid_bits:4; // 8 upon fetching a modbyte, if 0 fetch next
        uint16_t    modbits; // upto 0..8 modulation data bits, MSB aligned !
    } laser;


    // XXX: stepping profile: http://www.embedded.com/design/mcus-processors-and-socs/4006438/Generate-stepper-motor-speed-profiles-in-real-time

} state_t; // size=63 bytes: 1*4+3 bytes left



BUFFER_H(STEPPER_QUEUE, block_t, STEPPER_QUEUE_SIZE)

void stepper_clear_aborted(void);
void stepper_drain_buffer(void);
void stepper_init(void);
int32_t stepper_get_position(uint8_t idx);


#define USE_ARC_KERNEL
#define USE_HOME_KERNEL
#define USE_LINE_KERNEL
#define USE_OFF_TOOLBOX
#define USE_PWM_TOOLBOX
#define USE_PULSE_TOOLBOX


#endif
