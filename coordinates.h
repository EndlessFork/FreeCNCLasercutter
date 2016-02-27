#ifndef _COORDINATES_H_
#define _COORDINATES_H_

#include "platform.h"
#include "stepper.h"

number X_steps2position(StepperPos);
number Y_steps2position(StepperPos);
number Z_steps2position(StepperPos);

StepperPos X_position2steps(number);
StepperPos Y_position2steps(number);
StepperPos Z_position2steps(number);

#endif
