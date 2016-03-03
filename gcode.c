

#include "gcode.h"

#include "asm.h"
#include "time.h"
#include "laser.h"
#include "parser.h"
#include "planner.h"
#include "stepper.h"
#include "coordinates.h"

#include <stdlib.h>


/***********************************************************
 * GCODE handling
 * --------------
 * Gets called from parser.c upon end-of-line
 * converts positions from um to steps (with coordinates.*)
 * selects what to do and calls methods of planner
 ***********************************************************/

/******************************
 * NOTES:
 * ------
 * - speed/feedrates/accel are ignored atm.
 ******************************/

// Parser partially based upon Marlin

// look here for descriptions of gcodes: http://linuxcnc.org/handbook/gcode/g-code.html
// http://objects.reprap.org/wiki/Mendel_User_Manual:_RepRapGCodes
// http://reprap.org/wiki/G-code

//Implemented Codes
//-------------------
// G0  - move to new position as fast as possible
// G1  - Coordinated Movement X Y Z
// G2  - CW ARC
// G3  - CCW ARC
// G4  - Dwell S<seconds> or P<milliseconds>
// G5  - (optional) Beziercurve
// G7  - Laser RasterData
// G28 - Home all Axis
// G90 - Use Absolute Coordinates
// G91 - Use Relative Coordinates
// G92 - Set current position to cordinates given

// M Codes
// M0   - (exit SIM mode with error)
// M2   - (exit sim mode normally) END_OF_PROGRAM
// M3    - enable Tool (Laser)
// M5   - disable Tool (Laser)
// M400 - Finish all moves
// M649 - Setup properties for Laser Rastering
//
// planned to implement:
// M20  - List SD card
// M21  - Init SD card
// M22  - Release SD card
// M23  - Select SD file (M23 filename.g)
// M24  - Start/resume SD print
// M25  - Pause SD print
// M26  - Set SD position in bytes (M26 S12345)
// M27  - Report SD print status
// M28  - Start SD write (M28 filename.g)
// M29  - Stop SD write
// M30  - Delete file from SD (M30 filename.g)
// M31  - Output time since last M109 or SD card start to serial
// M32  - Select file and start SD print (Can be used when printing from SD card)
// M92  - Set Config.steps_per_unit - same syntax as G92
// M114 - Output current position to serial port
// M115 - Capabilities string
// M117 - display message
// M119 - Output Endstop status to serial port
// M201 - Set max Config.accel in units/s^2 for print moves (M201 X1000 Y1000)
// M202 - Set max Config.accel in units/s^2 for travel moves (M202 X1000 Y1000)
// M203 - Set maximum speed that your machine can sustain (M203 X200 Y200 Z300) in mm/sec
// M204 - Set default accel: S normal moves T laser moves (M204 S3000 T7000) im mm/sec^2
// M205 - advanced settings:  minimum travel G.speed S=while printing T=travel only,  B=minimum segment time X= maximum xy jerk, Z=maximum Z jerk, E=maximum E jerk
// M206 - set additional homeing offset
// M500 - stores paramters in EEPROM
// M501 - reads parameters from EEPROM (if you need reset them after you changed them temporarily).
// M502 - reverts to the default "factory settings".  You still need to store them in EEPROM afterwards if you want to.
// M503 - print the current settings (from memory not from eeprom)
// M540 - Use S[0|1] to enable or disable the stop SD card print on endstop hit (requires ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED)
// M999 - Restart after being stopped by error




gcode_t G;

/*******************************************************
 * HELPERS
 * -------
 * evaluate ccordinates for lines/moves/arcs
 * evaluate laser opts
 * write to relevant data structures in the right units
 *******************************************************/

// evaluate coordinates (XYZ)
void get_coordinates(void) {
    if (numbers_got & LETTER_X_MASK) {
        LOG_STRING("G: using specified X coordinate\n");
        if (G.axis_relative_mode[X_AXIS] || G.relative_mode)
            G.destination[X_AXIS] += numbers[LETTER_X];
        else
            G.destination[X_AXIS] = numbers[LETTER_X];
    }

    if (numbers_got & LETTER_Y_MASK) {
        LOG_STRING("G: using specified Y coordinate\n");
        if (G.axis_relative_mode[Y_AXIS] || G.relative_mode)
            G.destination[Y_AXIS] += numbers[LETTER_Y];
        else
            G.destination[Y_AXIS] = numbers[LETTER_Y];
    }

    if (numbers_got & LETTER_Z_MASK) {
        LOG_STRING("G: using specified Z coordinate\n");
        if (G.axis_relative_mode[Z_AXIS] || G.relative_mode)
            G.destination[Z_AXIS] += numbers[LETTER_Z];
        else
            G.destination[Z_AXIS] = numbers[LETTER_Z];
    }
    // transfer coordinates to planner
    PL.target[X_AXIS] = X_position2steps(G.destination[X_AXIS]);
    PL.target[Y_AXIS] = Y_position2steps(G.destination[Y_AXIS]);
    PL.target[Z_AXIS] = Z_position2steps(G.destination[Z_AXIS]);
}

// evaluate arc specifics (IJR)
// returns false, if this arc won't work
bool get_arc_coordinates(bool ccw) {
    // keep a reference of last destination
    G.arc_center_x = G.destination[X_AXIS];
    G.arc_center_y = G.destination[Y_AXIS];

    get_coordinates();

    if (0 == (numbers_got & (LETTER_I_MASK | LETTER_J_MASK | LETTER_R_MASK))) {
        LOG_STRING("Need at least one of I, J or R!");LOG_NEWLINE;
        return FALSE;
    }

    // Arc center is always relative to current position
    if (numbers_got & LETTER_I_MASK) { // center of arc in X
        G.arc_center_x += numbers[LETTER_I];
    }
    if (numbers_got & LETTER_J_MASK) { // center of arc in Y
        G.arc_center_y += numbers[LETTER_J];
    }
    LOG_STRING("G: ARC_center X:");LOG_S32(G.arc_center_x);LOG_NEWLINE;
    LOG_STRING("G: ARC_center Y:");LOG_S32(G.arc_center_y);LOG_NEWLINE;

    // calculate arc_center if not specified. uses the smaller arc if R>0, else the bigger one
    if (LETTER_R_MASK == (numbers_got & (LETTER_I_MASK|LETTER_J_MASK|LETTER_R_MASK))) { // neither I nor J given, but R
        number R = numbers[LETTER_R];
        // calculate arc_center!
        // G.destination holds the new target, G.arc_center_x still holds the old one (as I was not given)
        number dx = G.destination[X_AXIS] - G.arc_center_x;
        // G.destination holds the new target, G.arc_center_y still holds the old one (as J was not given)
        number dy = G.destination[Y_AXIS] - G.arc_center_y;
        // get length of direct line
        number dist = length2(dx, dy);
        // get height of triangle with sidelenghts R,dist,R
        //~ number h = sqrt(square(R) - square(dist/2));
        number h = _sqrt_u64( ((uint64_t) (((int64_t) R) * ((int64_t) R))) - ((uint64_t) (((int64_t) dist) * ((int64_t) dist)))/4);

        if (h < 0) {
            LOG_STRING("Impossible Arc! abort");LOG_NEWLINE;
            return FALSE;
        }
        LOG_STRING("G: get_arc_center: dx:");LOG_S32(dx);LOG_NEWLINE;
        LOG_STRING("G: get_arc_center: dy:");LOG_S32(dy);LOG_NEWLINE;
        LOG_STRING("G: get_arc_center: dist:");LOG_S32(dist);LOG_NEWLINE;
        LOG_STRING("G: get_arc_center: h:");LOG_S32(h);LOG_NEWLINE;
        // calculate 1st part of final arc_center
        G.arc_center_x = dy * h / dist;
        G.arc_center_y = -dx * h / dist;
        LOG_STRING("G: get_arc_center: relative X:");LOG_S32(G.arc_center_x);LOG_NEWLINE;
        LOG_STRING("G: get_arc_center: relative Y:");LOG_S32(G.arc_center_y);LOG_NEWLINE;

        // negative radii indicate arc_angles above 180°, fixing those up
        if ((ccw && (R > 0)) || (!ccw && (R < 0))) {
            LOG_STRING("G: get_src_center: negate!\n");
            G.arc_center_x = -G.arc_center_x;
            G.arc_center_y = -G.arc_center_y;
        }
        LOG_STRING("G: get_arc_center: relative X:");LOG_S32(G.arc_center_x);LOG_NEWLINE;
        LOG_STRING("G: get_arc_center: relative Y:");LOG_S32(G.arc_center_y);LOG_NEWLINE;
        // now we should add the midpoint between current destination and last destination (current starting point)
        // dx/dy holds destination - start, we need destination + start, so take destination - dx/dy
        G.arc_center_x += (2 * G.destination[X_AXIS] - dx) / 2;
        G.arc_center_y += (2 * G.destination[Y_AXIS] - dy) / 2;
    }
    LOG_STRING("G: ARC_center X:");LOG_S32(G.arc_center_x);LOG_NEWLINE;
    LOG_STRING("G: ARC_center Y:");LOG_S32(G.arc_center_y);LOG_NEWLINE;

    // transfer coordinates to planner. NOTE: Radius is auto-calculated by planner, but center needs to be correct.
    PL.arc_center_x = X_position2steps(G.arc_center_x);
    PL.arc_center_y = Y_position2steps(G.arc_center_y);
    return TRUE;
}

// evaluate laser options (BCLPQS)
void handle_laser_opts(void) {
    if (numbers_got & LETTER_S_MASK) {
        laser_set_power(min(1000, max(0, numbers[LETTER_S] / 100))); // Value given in %: convert to promille
        // switch to CW
        laser_set_mode(LASER_CW);
    }
    if (numbers_got & LETTER_L_MASK) {
        laser_set_pulse_us(labs(numbers[LETTER_L])); // given in ms*1000, evaluated in us
        laser_set_mode(LASER_PULSE);
    }
    if (numbers_got & LETTER_P_MASK) {
        laser_set_ppm(numbers[LETTER_P]); // pulses per millimeter *1000 = pulses per meter
        laser_set_mode(LASER_PULSE);
    }
    if (numbers_got & LETTER_C_MASK) {
        laser_set_mode(laser.mode | LASER_MODULATE);
        switch(integers[LETTER_C]) { // modulation
            case 1 : laser_set_modulation(LASER_MODULATION_1BIT);break;
            case 2 : laser_set_modulation(LASER_MODULATION_2BIT);break;
            case 3 : laser_set_modulation(LASER_MODULATION_3BIT);break;
            case 4 : laser_set_modulation(LASER_MODULATION_4BIT);break;
            case 6 : laser_set_modulation(LASER_MODULATION_6BIT);break;
            case 8 : laser_set_modulation(LASER_MODULATION_8BIT);break;
            default : laser_set_mode(LASER_OFF);
        }
    }
    if (numbers_got & LETTER_B_MASK) laser_set_mode(integers[LETTER_B]); // mode?

    // request a specific amount of pulses, or (0) auto calculate from length and laser.ppm
    laser.pulses = (numbers_got & LETTER_Q_MASK) ? integers[LETTER_Q] : 0;

    // XXX: check for presence of T and select apropriate Tool (copy laser settings)
}


// called from parser upon end-of-line
// evaluates command and calls appropriate helpers
void process_command() {
    uint32_t tmp;

    if (numbers_got & LETTER_F_MASK) { // always handle F first
        if (numbers[LETTER_F] > 0)
            G.speed = max((numbers[LETTER_F] + 30) / 60, DEFAULT_MIN_SPEED);
            LOG_STRING("G: set_speed to um/s: ");LOG_U32(G.speed);LOG_NEWLINE;
    }
    if (numbers_got & LETTER_G_MASK) { // handle G-codes
        LOG_STRING("G: handling G");LOG_U8(integers[LETTER_G]);LOG_NEWLINE;
        // ignore G-codes above 255
        if (!(integers[LETTER_G] & 0xffffff00))
        switch ((uint8_t)integers[LETTER_G]) {
            case 0: // G0 - move
                get_coordinates();
                planner_move(G.speed);
                break;

            case 1: // G1 - line
                get_coordinates(); // For X Y Z
                handle_laser_opts();
                planner_line(G.speed);
                break;

            case 2: // G2  - CW ARC
                PERF_PLABEL(2, PSTR("CW_ARC"));
                PERF_START(2);
                if (!get_arc_coordinates(FALSE)) {
                    // skip bad arcs and move to next coordinate
                    planner_move(G.speed);
                } else {
                    handle_laser_opts();
                    planner_arc(G.speed, FALSE);
                }
                PERF_STOP(2);
                break;

            case 3: // G3  - CCW ARC
                PERF_PLABEL(3, PSTR("CCW_ARC"));
                PERF_START(3);
                if (!get_arc_coordinates(TRUE)) {
                    // skip bad arcs and move to next coordinate
                    planner_move(G.speed);
                } else {
                    handle_laser_opts();
                    planner_arc(G.speed, TRUE);
                }
                PERF_STOP(3);
                break;

            case 4: // G4 dwell
                //LCD_MESSAGEPGM(MSG_DWELL);
                tmp = 0;
                if (numbers_got & LETTER_P_MASK) tmp = integers[LETTER_P]; // milliseconds to wait
                if (numbers_got & LETTER_S_MASK) tmp += numbers[LETTER_S]; // seconds to wait

                stepper_drain_buffer();
                tmp += millis();  // keep track of when we started waiting
                while ((tmp - millis()) > 0 ){
                    idle('D');
                }
                break;

            #ifdef G5_BEZIER
            case 5: // G5 bezier curve - from http://forums.reprap.org/read.php?147,93577
                float p[4][2] = {{0.0,0.0},{0.0,0.0},{0.0,0.0},{0.0,0.0}};
                int16_t steps = 10;
                float stepsPerUnit = 1;
                float f[2]={0,0};
                float fd[2]={0,0};
                float fdd[2]={0,0};
                float fddd[2]={0,0};
                float fdd_per_2[2]={0,0};
                float fddd_per_2[2]={0,0};
                float fddd_per_6[2]={0,0};
                float t = (1.0);
                float temp;
                // get coordinates
                //---------------------------------------
                // start point
                p[0][0] = PL.position[0];
                p[0][1] = PL.position[1];
                // control point 1
                if (codes_seen & LETTER_I_MASK) p[1][0] = floats[LETTER_I] + (G.axis_relative_modes[0] || G.relative_mode)*PL.position[0];
                if (codes_seen & LETTER_J_MASK) p[1][1] = floats[LETTER_J] + (G.axis_relative_modes[1] || G.relative_mode)*PL.position[1];

                // control point 2
                if (codes_seen & LETTER_K_MASK) p[2][0] = floats[LETTER_K] + (G.axis_relative_modes[0] || G.relative_mode)*PL.position[0];
                if (codes_seen & LETTER_L_MASK) p[2][1] = floats[LETTER_L] + (G.axis_relative_modes[1] || G.relative_mode)*PL.position[1];
                // end point
                if (codes_seen & LETTER_X_MASK) p[3][0] = floats[LETTER_X] + (G.axis_relative_modes[0] || G.relative_mode)*PL.position[0];
                if (codes_seen & LETTER_Y_MASK) p[3][1] = floats[LETTER_Y] + (G.axis_relative_modes[1] || G.relative_mode)*PL.position[1];

                // calc num steps
                float maxD = 0, sqrD = 0;
                for (int16_t i=1; i<4; i++) {
                    sqrD = (p[i][0] - p[i-1][0])*(p[i][0] - p[i-1][0]) + (p[i][1] - p[i-1][1])*(p[i][1] - p[i-1][1]);
                    if (sqrD > maxD) {maxD = sqrD; };
                }
                maxD = sqrt(maxD);
                if (maxD > 0) {
                    steps = round((3 * maxD * stepsPerUnit));
                }
                if (steps < 1) steps = 1;
                if (steps > 200) steps = 200;
                // init Forward Differencing algo
                //---------------------------------------
                t = 1.0 / steps;
                temp = t*t;
                for (int16_t i=0; i<2; i++) {
                    f[i] = p[0][i];
                    fd[i] = 3 * (p[1][i] - p[0][i]) * t;
                    fdd_per_2[i] = 3 * (p[0][i] - 2 * p[1][i] + p[2][i]) * temp;
                    fddd_per_2[i] = 3 * (3 * (p[1][i] - p[2][i]) + p[3][i] - p[0][i]) * temp * t;

                    fddd[i] = fddd_per_2[i] + fddd_per_2[i];
                    fdd[i] = fdd_per_2[i] + fdd_per_2[i];
                    fddd_per_6[i] = (fddd_per_2[i] * (1.0 / 3));
                }
                // prep PL.target
                for(int16_t i=0; i < NUM_AXIS; i++) {
                    PL.target[i] = PL.position[i];
                }
                // iterate through curve
                //---------------------------------------
                for (int16_t loop=0; loop < steps; loop++) {
                    PL.target[0] = f[0];
                    PL.target[1] = f[1];
                    planner_move(G.speed);
                    previous_millis_cmd = millis();

                    // update f
                    for (int16_t i=0; i<2; i++) {
                        f[i] = f[i] + fd[i] + fdd_per_2[i] + fddd_per_6[i];
                        fd[i] = fd[i] + fdd[i] + fddd_per_2[i];
                        fdd[i] = fdd[i] + fddd[i];
                        fdd_per_2[i] = fdd_per_2[i] + fddd_per_2[i];
                    }
                }
                // Move to final position
                PL.target[0] = p[3][0];
                PL.target[1] = p[3][1];
                planner_line(G.speed);
                previous_millis_cmd = millis();
                break;
            #endif // G5_BEZIER

            case 7: // G7 Execute raster line: XXX Rework this!
                LOG_STRING("G: G7 found: curent position is ");
                LOG_U32(PL.position[X_AXIS]);LOG_COMMA;
                LOG_U32(PL.position[Y_AXIS]);LOG_COMMA;
                LOG_U32(PL.position[Z_AXIS]);LOG_NEWLINE;

                //~ if (codes_seen & LETTER_D_MASK) { // direction (left/right) + 1 step in Y
                    //~ LOG_STRING("G: advance one step in Y\n");
                    //~ laser_set_raster_direction((integers[LETTER_D])?1:0);
                    //~ PL.target[Y_AXIS] = PL.position[Y_AXIS] + (1000000 / laser.ppm); // increment Y axis
                    //~ LOG_STRING("G: new y is ");LOG_U32(PL.target[Y_AXIS]);LOG_NEWLINE;
                //~ }
                if (base64_len) {  // Base64 encoded raster data (max 51 pixels)
                    uint8_t i;
                    LOG_STRING("G: Got PixelData, Bytse: ");LOG_U8(base64_len);LOG_NEWLINE;
                    laser.pulses = base64_len;
                    laser_set_raster_num_pixels(base64_len);
                    // copy pixeldata
                    for(i=0;i<base64_len;i++) LASER_RASTERDATA_put(base64_bytes[i]);
                }
                //~ if (laser.raster_direction) {
                    //~ PL.target[X_AXIS] = PL.position[X_AXIS] + (1000000 / laser.ppm * laser.raster_num_pixels);
                    //~ LOG_STRING("G: Positive Raster Line to X:");LOG_U32(PL.target[X_AXIS]);LOG_NEWLINE;
                //~ } else {
                    //~ PL.target[X_AXIS] = PL.position[X_AXIS] - (1000000 / laser.ppm * laser.raster_num_pixels);
                    //~ LOG_STRING("G: Negative Raster Line to X:");LOG_U32(PL.target[X_AXIS]);LOG_NEWLINE;
                //~ }
                if (numbers_got & LETTER_Q_MASK)
                    laser.pulses = integers[LETTER_Q]; // override base64_len

                laser_set_pulse_us((1000000 / G.speed) / laser.ppm); // (1 second in microseconds / (time to move 1mm in microseconds)) / (pulses per mm) = Duration of pulse, taking into account G.speed as G.speed and ppm
                LOG_STRING("G: Setting pulse_us: ");LOG_U32(laser.pulse_duration_us);LOG_NEWLINE;

                laser_set_mode(LASER_RASTER_CW); //XXX: support RASTER_PULSED as well !
                planner_line(G.speed);
                break;

            case 28: //G28 Home (all) Axis
            {
                uint8_t axis_mask = 0;
                idle('H');
                LOG_STRING("G: HOMING\n");

                if (!(codes_seen & (LETTER_X_MASK | LETTER_Y_MASK | LETTER_Z_MASK))) {
                    // home all axes
                    axis_mask = X_AXIS_MASK | Y_AXIS_MASK | Z_AXIS_MASK;
                } else {
                    // home specific axes
                    if (codes_seen & LETTER_X_MASK) axis_mask |= X_AXIS_MASK;
                    if (codes_seen & LETTER_Y_MASK) axis_mask |= Y_AXIS_MASK;
                    if (codes_seen & LETTER_Z_MASK) axis_mask |= Z_AXIS_MASK;
                }
                // ignore homing on axes which can not home (*_HOME_DIR == 0)
                axis_mask &=~( 0
                #if X_HOME_DIR == 0
                    | X_AXIS_MASK
                #endif
                #if Y_HOME_DIR == 0
                    | Y_AXIS_MASK
                #endif
                #if Z_HOME_DIR == 0
                    | Z_AXIS_MASK
                #endif
                    );

                if (axis_mask) {
                    LOG_STRING("G28: axis_mask is");LOG_U8(axis_mask);LOG_NEWLINE;
                    if (axis_mask & X_AXIS_MASK) {
                        // if homing X, set destination
                        // if homed position is specified, use that, else the configured value
                        G.destination[X_AXIS] = (numbers_got & LETTER_X_MASK) ? numbers[LETTER_X] : ((number) ONE * (number) X_HOME_POS);
                        LOG_STRING("G28: X->");LOG_S32(G.destination[X_AXIS]);LOG_NEWLINE;
                    }
                    if (axis_mask & Y_AXIS_MASK) {
                        G.destination[Y_AXIS] = (numbers_got & LETTER_Y_MASK) ? numbers[LETTER_Y] : ((number) ONE * (number) Y_HOME_POS);
                        LOG_STRING("G28: Y->");LOG_S32(G.destination[Y_AXIS]);LOG_NEWLINE;
                    }
                    if (axis_mask & Z_AXIS_MASK) {
                        G.destination[Z_AXIS] = (numbers_got & LETTER_Z_MASK) ? numbers[LETTER_Z] : ((number) ONE * (number) Z_HOME_POS);
                        LOG_STRING("G28: Z->");LOG_S32(G.destination[Z_AXIS]);LOG_NEWLINE;
                    }
                    // transfer coordinates to planner
                    PL.target[X_AXIS] = X_position2steps(G.destination[X_AXIS]);
                    PL.target[Y_AXIS] = Y_position2steps(G.destination[Y_AXIS]);
                    PL.target[Z_AXIS] = Z_position2steps(G.destination[Z_AXIS]);
                    LOG_STRING("G28: PL_target is");LOG_S24(PL.target[X_AXIS]);LOG_S24(PL.target[Y_AXIS]);LOG_S24(PL.target[Z_AXIS]);LOG_NEWLINE;

                    planner_home_axes(axis_mask);
                    LOG_STRING("G: HOMING DONE\n");
                } else {
                    LOG_STRING("G: No Homing Axes specified\n");
                }
            }
                stepper_drain_buffer();
                G.relative_mode = FALSE;
                break;

            case 90: // G90 - absolute coordinates
                G.relative_mode = FALSE;
                break;

            case 91: // G91 - relative coordinates
                G.relative_mode = TRUE;
                break;

            case 92: // G92 - set current position
                // XXX: can we use get_coordinates here?
                if (numbers_got & LETTER_X_MASK) G.destination[X_AXIS] = numbers[LETTER_X];
                if (numbers_got & LETTER_Y_MASK) G.destination[Y_AXIS] = numbers[LETTER_Y];
                if (numbers_got & LETTER_Z_MASK) G.destination[Z_AXIS] = numbers[LETTER_Z];

                // transfer coordinates to planner
                PL.target[X_AXIS] = X_position2steps(G.destination[X_AXIS]);
                PL.target[Y_AXIS] = Y_position2steps(G.destination[Y_AXIS]);
                PL.target[Z_AXIS] = Z_position2steps(G.destination[Z_AXIS]);

                planner_home_axes(0); // no homing, but updating of position
                break;

        } // case (integers[LETTER_G])
    } else if ((numbers_got & LETTER_M_MASK) && !(integers[LETTER_M] & 0xffff0000)) { // handle M-codes second
        switch ((uint16_t) integers[LETTER_M]) {
            case 0: // M0 - undefined
                LOG_STRING("G: M command without number!!!\n");

            case 2: // M2 - exit program
                LOG_STRING("G: exiting Program\n");
                laser_set_mode(LASER_OFF);
                planner_move(G.speed);
                stepper_drain_buffer();
                PERF_DUMP_ALL;
                avrtest_abort();
                // if running on real HW, continue program
                break;

            case 3:  //M3 - fire laser
                handle_laser_opts();
                if (!laser.mode)
                    laser_set_mode(LASER_CW);
                planner_move(G.speed);
                break;

            case 5:  //M5 stop firing laser
                {   uint8_t tmp = laser.mode;
                    laser_set_mode(LASER_OFF);
                    planner_move(G.speed);
                    laser_set_mode(tmp);
                }
                break;

            #ifdef SDSUPPORT
            case 20: // M20 - list SD card
                SERIAL_PROTOCOLLNPGM(MSG_BEGIN_FILE_LIST);
                card.ls();
                SERIAL_PROTOCOLLNPGM(MSG_END_FILE_LIST);
                break;

            case 21: // M21 - init SD card
                card.initsd();
                break;

            case 22: //M22 - release SD card
                card.release();
                break;

            case 23: //M23 - Select file
                // XXX change to using the filename from parser.c directly!
                starpos = (strchr(strchr_pointer + 4,'*'));
                if (starpos!=NULL)
                *(starpos-1)='\0';
                card.openFile(strchr_pointer + 4,TRUE);
                break;

            case 24: //M24 - Start SD print
                card.startFileprint();
                starttime=millis();
                break;

            case 25: //M25 - Pause SD print
                card.pauseSDPrint();
                break;

            case 26: //M26 - Set SD index
                if (card.cardOK && code_seen('S')) {
                    card.setIndex(code_value_int());
                }
                break;

            case 27: //M27 - Get SD status
                card.getStatus();
                break;

            case 28: //M28 - Start SD write
                starpos = (strchr(strchr_pointer + 4,'*'));
                if (starpos != NULL){
                    uint8_t* npos = strchr(cmdbuffer[bufindr], 'N');
                    strchr_pointer = strchr(npos,' ') + 1;
                    *(starpos-1) = '\0';
                }
                card.openFile(strchr_pointer+4,FALSE);
                break;

            case 29: //M29 - Stop SD write
                //processed in write to file routine above
                //card,saving = FALSE;
                break;

            case 30: //M30 <filename> Delete File
                if (card.cardOK) {
                    card.closefile();
                    starpos = (strchr(strchr_pointer + 4,'*'));
                    if (starpos != NULL){
                        uint8_t* npos = strchr(cmdbuffer[bufindr], 'N');
                        strchr_pointer = strchr(npos,' ') + 1;
                        *(starpos-1) = '\0';
                    }
                    card.removeFile(strchr_pointer + 4);
                }
                break;

            case 32: //M32 - Select file and start SD print
                if (card.sdprinting) {
                    stepper_synchronize();
                    card.closefile();
                    card.sdprinting = FALSE;
                }
                starpos = (strchr(strchr_pointer + 4,'*'));
                if (starpos!=NULL)
                    *(starpos-1)='\0';
                card.openFile(strchr_pointer + 4,TRUE);
                card.startFileprint();
                starttime=millis();
                break;

            case 928: //M928 - Start SD write
                starpos = (strchr(strchr_pointer + 5,'*'));
                if (starpos != NULL){
                    uint8_t* npos = strchr(cmdbuffer[bufindr], 'N');
                    strchr_pointer = strchr(npos,' ') + 1;
                    *(starpos-1) = '\0';
                }
                card.openLogFile(strchr_pointer+5);
                break;

            #endif //SDSUPPORT

            case 400: // M400 finish all moves
                stepper_drain_buffer();
                break;

            //~ case 500: // M500 Store settings in EEPROM
                //~ Config_StoreSettings();
                //~ break;

            //~ case 501: // M501 Read settings from EEPROM
                //~ Config_RetrieveSettings();
                //~ break;

            //~ case 502: // M502 Revert to default settings
                //~ Config_ResetDefault();
                //~ break;

            //~ case 503: // M503 print settings currently in memory
                //~ Config_PrintSettings();
                //~ break;

            case 649: // M649 set laser options
                handle_laser_opts();
                break;

        } // switch (integers[LETTER_M])
    } else if (numbers_got & LETTER_T_MASK) {// Tool change (extruder change for 3D printers, lasers ignore this
#if 0
        tmp_extruder = integers[LETTER_T];
        // do something? parse laser settings and store as tool T ?
#endif
    } else if (codes_seen & LETTER_V_MASK) { // request Version info
        cmd_print(PSTR(VERSION));
        LOG_STRING(VERSION "\n");
    } else {
        // ignore other commands
    }

}

