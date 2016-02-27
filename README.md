# FreeCNCLasercutter

Attempt to provide a Firmware for the FreeCNC Board controlling a chinese K40
type lasercutter.

First, I wanted to adopt http://github.com/TurnkeyTyranny/buildlog-lasercutter-marlin
which is Marlin (i.e. 3D-printer) based.

I'm in the process of rewriting everything.
Except the usb_serial.c which is copyright from pjrc.com and slightly adopted.
(Originally for the Teensy 2.0 (Which I love! Tanks Paul!) but since
the FreeCNC board uses the same chip....)
(Part of the development was done on a Teensy 2.)

Documentation is in directory doc and the copyright lies with the respective owners, not by me.
It is included for educational puposes only.

further links:
reprap-gcode table
inkscape plugin


Internal values are in micrometers or in micrometers per second to get rid of floats.
Per length values are in count per meter.
The parser works on a char-by-char basis.
Units are in millimeters/Inches and 3 subdigits are handled correctly.
(inches are handled by multiplying the 'parsed-value' by 25.4 to get the 'mm-value')

internal datatype is int32_t, stepper positions are in int24_t.
The resulting value range is much bigger than needed, but avoids overflows.

goodies:
 - uses avrtest for simulation runs (see: http://sourceforge.net/p/winavr/code/HEAD/tree/trunk/avrtest/)

missing:
 - stepping interrupt routine needs a rework (implement the method from http://www.embedded.com/design/mcus-processors-and-socs/4006438/Generate-stepper-motor-speed-profiles-in-real-time )
 - efficient square root /vector length calculation for vectors
 - probably rewriting everything again :(
