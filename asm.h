
#ifndef _asm_h_
#define _asm_h_

#include "platform.h"


/*********************************
 * Fancy optimizing ASM routines *
 *********************************/


/*
 * multiplies two uint16_t values and returns the higher 16 bits as uint16_t
 */
uint16_t mul16SHR16(uint16_t a, uint16_t b);
uint32_t _sqrt_u64(uint64_t num);
uint32_t length2(int32_t x, int32_t y);
number length3(number x, number y, number z);


#ifndef OPTIMIZE_ASM
#define ADVANCE_POSITION(ctr, dir) ctr+=dir;
#else
// shorter form of "ctr += dir" for dir=(int8_t) +1 or -1 and ctr=int24_t
#define ADVANCE_POSITION(ctr, dir) \
asm volatile ( \
    "mov    __tmp_reg__, %A1    \n\t" \
    "asr    __tmp_reg__     \n\t"   /* convert -1 to 0xff and +1 to 0 */ \
    "add    %A0, %A1    \n\t"           /* 24bit add of either 0xffffff or 0x000001 */ \
    "adc    %B0, __tmp_reg__    \n\t" \
    "adc    %C0, __tmp_reg__    \n\t" \
: \
"+r" ((int24_t) ctr) \
: \
"r" ((int8_t) dir) \
)
#endif // !OPTIMIZE_ASM




#ifndef OPTIMIZE_ASM
#define ADVANCE_STEP_COUNTER(ctr, add, sub, dostep) {dostep=0;ctr+=add;if (ctr>0) {dostep=1;ctr-=sub;}}
#else
// shorter form of "dostep=0; ctr += add; if (ctr>0) {dostep=1;ctr-=sub}"
#define ADVANCE_STEP_COUNTER(ctr, add, sub, dostep) \
asm volatile ( \
    "clr    %A1     \n\t"       /* dostep=0, also C=0 */ \
    "add    %A0, %A2    \n\t"   /* 24it ADD */ \
    "adc    %B0, %B2    \n\t" \
    "adc    %C0, %C2    \n\t" \
    "brcc   0f  \n\t"           /* no Carry-> skip */ \
    "inc    %A1 \n\t"           /* dostep = 1 now */ \
    "sub    %A0, %A3    \n\t"   /* 24bit SUB */ \
    "sbc    %B0, %B3    \n\t" \
    "sbc    %C0, %C3    \n\t" \
    "0: \n\t"\
: \
"+r" ((int24_t) ctr), \
"=&r" ((uint8_t) dostep) \
: \
"r" ((uint24_t) add), \
"r" ((uint24_t) sub) \
)
#endif // !OPTIMIZE_ASM





#ifndef OPTIMIZE_ASM
#define times_ten(x) x*=10
#else
#define times_ten_(x) x*=10
#define times_ten(dword) \
asm volatile ( \
"lsl %A0 \n\t" \
"rol %B0 \n\t" \
"rol %C0 \n\t" \
"rol %D0 \n\t" \
"push %D0 \n\t" \
"push %C0 \n\t" \
"push %B0 \n\t" \
"push %A0 \n\t" \
"lsl %A0 \n\t" \
"rol %B0 \n\t" \
"rol %C0 \n\t" \
"rol %D0 \n\t" \
"lsl %A0 \n\t" \
"rol %B0 \n\t" \
"rol %C0 \n\t" \
"rol %D0 \n\t" \
"pop __tmp_reg__ \n\t" \
"add %A0, __tmp_reg__ \n\t" \
"pop __tmp_reg__ \n\t" \
"adc %B0, __tmp_reg__ \n\t" \
"pop __tmp_reg__ \n\t" \
"adc %C0, __tmp_reg__ \n\t" \
"pop __tmp_reg__ \n\t" \
"adc %D0, __tmp_reg__ \n\t" \
: \
"+r" (dword) \
: \
"0" (dword) \
: \
"r0" \
)
#endif // !OPTIMIZE_ASM


#endif
