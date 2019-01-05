/** @file 
 * Ethereum Nanon verification.
 * */

#ifndef in3_bignum_h__
#define in3_bignum_h__

#include <stdint.h>

void big_shift_left(uint8_t* a, int len, int bits);
void big_shift_right(uint8_t* a, int len, int bits);
int  big_cmp(uint8_t* a, int len_a, uint8_t* b, int len_b);
int  big_signed(uint8_t* val, int len, uint8_t* dst);
int  big_int(uint8_t* val, int len);
#endif