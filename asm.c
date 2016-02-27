
#include "asm.h"

/*********************************
 * Fancy optimising ASM routines *
 *********************************/

/*
 * multiplies two uint16_t values and returns the higher 16 bits as uint16_t
 */

uint16_t mul16SHR16(uint16_t a, uint16_t b) {
#ifndef OPTIMIZE_ASM
    return (((uint32_t) a) * ((uint32_t) b)) >> 16;
#else
    uint16_t temp;
    uint16_t result;
    // berechne (a*b) >> 16
    asm volatile (
    "clr %A0 \n\t"
    "clr %A1 \n\t"
    "clr %B1 \n\t"

    "mul %A2, %A3 \n\t"
    "mov %B0, r1 \n\t"

    "mul %A2, %B3 \n\t"
    "add %B0, r0 \n\t"
    "adc %A1, r1 \n\t"
    "adc %B1, %A0 \n\t"

    "mul %B2, %A3 \n\t"
    "add %B0, r0 \n\t"
    "adc %A1, r1 \n\t"
    "adc %B1, %A0 \n\t"

    "mul %B2, %B3 \n\t"
    "add %A1, r0 \n\t"
    "adc %B1, r1 \n\t"

    "clr __zero_reg__ \n\t"

    :
    "=&r" (temp),
    "=&r" (result)
    :
    "r" (a),
    "r" (b)
    );
    return result;
    #endif // !OPTIMIZE_ASM
}


// from: https://en.wikipedia.org/wiki/Methods_of_computing_square_roots
uint32_t _sqrt_u64(uint64_t num) {
    uint64_t res = 0;
    uint64_t bit = 1ULL << 62; // The second-to-top bit is set: 1 << 30 for 32 bits

    // "bit" starts at the highest power of four <= the argument.
    while (bit > num)
        bit >>= 2;

    while (bit != 0) {
        if (num >= res + bit) {
            num -= res + bit;
            res = (res >> 1) + bit;
        }
        else
            res >>= 1;
        bit >>= 2;
    }
    return res;
}

// XXX: optimize this!
uint32_t length2(int32_t x, int32_t y) {
    uint32_t res = _sqrt_u64(((uint64_t) (((int64_t) x) * ((int64_t) x))) + ((uint64_t) (((int64_t) y) * ((int64_t) y))));
    LOG_STRING("PL:length2(");LOG_S32(x);LOG_S32(y);LOG_STRING(")\n =>");LOG_U32(res);LOG_NEWLINE;
    return res;
}

#if 0
/*****************
 * binary search * // may use the HW multiplier!
 *****************/
uint8_t bs_sqrt_u16(uint16_t num) {
    uint8_t mask = 0x40;
    uint8_t res = 0x80;
    uint16_t sq;

    while (mask) {
        sq = res*res;
        if (sq > num)
            res -= mask;
        else
            res += mask;
        mask >>= 1;
    }
    return res;
}

uint16_t bs_sqrt_u32(uint32_t num) {
    uint16_t mask = 0x8000UL;
    uint16_t res = 0;
    uint32_t sq;

    // note: mask may start at a lower bit position if num is lower
    while (mask) {
        res += mask;
        sq = res*res;
        if (sq > num)
            res -= mask;
        mask >>= 1;
    }
    return res;
}

uint32_t bs_sqrt_u64(uint64_t num) {
    uint32_t mask = 0x80000000UL;
    uint32_t res = 0;
    uint64_t sq;

    // note: mask may start at a lower bit position if num is lower
    while (mask) {
        res += mask;
        sq = res*res;
        if (sq > num)
            res -= mask;
        mask >>= 1;
    }
    return res;
}

// another try:
uint32_t ef_sqrt_u64(uint64_t num) {
    uint16_t mask = 0x8000UL;
    uint32_t res = 0;
    uint64_t sq;

    res = (uint32_t) bs_sqrt_u32(num >> 32UL) << 16UL;
    while (mask) {
        res += mask;
        sq = res*res;
        if (sq > num)
            res -= mask;
        mask >>= 1;
    }
    return res;
}
#endif
