; full line comments

; rounded square
G0 X5 Y0 Z0 ; partial comment
M649 S13.7 L2 P5 B2 C1(short comments DO NOT NEST!)
G1 X10
G3 X10 I0 J5;full circle
G1 X15
G3 X20 Y5 I0 J5 S42 R5
G1 Y15 L3
G3 X15 Y20 I-5 P10; omitting J0
G1 X5 P5
G3 X0 Y15 J-5 S210
G1 Y5
G3 X5 Y0 R5;I5

M0 (finito)
