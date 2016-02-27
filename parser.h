#ifndef _PARSER_H_
#define _PARSER_H_

#include "platform.h"


#define LETTER_A  0
#define LETTER_B  1
#define LETTER_C  2
#define LETTER_D  3
#define LETTER_E  4
#define LETTER_F  5
#define LETTER_G  6
#define LETTER_H  7
#define LETTER_I  8
#define LETTER_J  9
#define LETTER_K 10
#define LETTER_L 11
#define LETTER_M 12
#define LETTER_N 13
#define LETTER_O 14
#define LETTER_P 15
#define LETTER_Q 16
#define LETTER_R 17
#define LETTER_S 18
#define LETTER_T 19
#define LETTER_U 20
#define LETTER_V 21
#define LETTER_W 22
#define LETTER_X 23
#define LETTER_Y 24
#define LETTER_Z 25

#define LETTER_A_MASK ((uint32_t)1<< 0)
#define LETTER_B_MASK ((uint32_t)1<< 1)
#define LETTER_C_MASK ((uint32_t)1<< 2)
#define LETTER_D_MASK ((uint32_t)1<< 3)
#define LETTER_E_MASK ((uint32_t)1<< 4)
#define LETTER_F_MASK ((uint32_t)1<< 5)
#define LETTER_G_MASK ((uint32_t)1<< 6)
#define LETTER_H_MASK ((uint32_t)1<< 7)
#define LETTER_I_MASK ((uint32_t)1<< 8)
#define LETTER_J_MASK ((uint32_t)1<< 9)
#define LETTER_K_MASK ((uint32_t)1<<10)
#define LETTER_L_MASK ((uint32_t)1<<11)
#define LETTER_M_MASK ((uint32_t)1<<12)
#define LETTER_N_MASK ((uint32_t)1<<13)
#define LETTER_O_MASK ((uint32_t)1<<14)
#define LETTER_P_MASK ((uint32_t)1<<15)
#define LETTER_Q_MASK ((uint32_t)1<<16)
#define LETTER_R_MASK ((uint32_t)1<<17)
#define LETTER_S_MASK ((uint32_t)1<<18)
#define LETTER_T_MASK ((uint32_t)1<<19)
#define LETTER_U_MASK ((uint32_t)1<<20)
#define LETTER_V_MASK ((uint32_t)1<<21)
#define LETTER_W_MASK ((uint32_t)1<<22)
#define LETTER_X_MASK ((uint32_t)1<<23)
#define LETTER_Y_MASK ((uint32_t)1<<24)
#define LETTER_Z_MASK ((uint32_t)1<<25)

extern uint32_t codes_seen; // A is at bit 0
extern uint32_t numbers_got; // A is at bit 0
// result storage
extern number numbers[26];
extern int32_t integers[26];

extern uint8_t base64_bytes[LASER_MAX_RASTER_LINE];
extern uint8_t base64_len; // amount of valid bytes in base64_bytes
extern char filename[16]; // filename for M20..M33 commands
extern uint8_t filename_len; // amount of valid characters in filename

void parser_init(void);
bool process_char(char c);

#endif
