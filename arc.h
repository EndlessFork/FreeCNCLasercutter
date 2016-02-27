
#ifndef _arc_h_
#define _arc_h_

#include "platform.h"
#include "stepper.h"

// coordinates are relative to the center of the arc!
// this will also send the rigth buffer block to the stepper queue
void buffer_arc(uint32_t R, StepperPos fromx, StepperPos fromy,
                                   StepperPos tox, StepperPos toy, bool ccw);

#endif
