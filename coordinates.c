

#include "coordinates.h"

// we have equal steps_per_unit for X and Y. if not, adopt arc-kernel as well!!!

number X_steps2position(StepperPos x) {
    return ((int64_t) x * 1000000UL) / XY_AXIS_STEPS_PER_M;
}

number Y_steps2position(StepperPos y) {
    return ((int64_t) y * 1000000UL) / XY_AXIS_STEPS_PER_M;
}

number Z_steps2position(StepperPos z) {
    return ((int64_t) z * 1000000UL) / Z_AXIS_STEPS_PER_M;
}


StepperPos X_position2steps(number x) {
    return ( ((int64_t) x) * ((int64_t) XY_AXIS_STEPS_PER_M) ) / 1000000L;
}

StepperPos Y_position2steps(number y) {
    return ( ((int64_t) y) * ((int64_t) XY_AXIS_STEPS_PER_M) ) / 1000000L;
}

StepperPos Z_position2steps(number z) {
    return ( ((int64_t) z) * ((int64_t) Z_AXIS_STEPS_PER_M) ) / 1000000L;
}

