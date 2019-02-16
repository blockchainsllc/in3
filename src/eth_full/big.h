/** @file 
 * Ethereum Nanon verification.
 * */

#ifndef in3_bignum_h__
#define in3_bignum_h__

#include <stdint.h>

uint8_t big_is_zero(uint8_t* data, uint_fast8_t l);
void    big_shift_left(uint8_t* a, uint_fast8_t len, int bits);
void    big_shift_right(uint8_t* a, uint_fast8_t len, int bits);
int     big_cmp(uint8_t* a, uint_fast8_t len_a, uint8_t* b, uint_fast8_t len_b);
int     big_signed(uint8_t* val, uint_fast8_t len, uint8_t* dst);
int32_t big_int(uint8_t* val, uint_fast8_t len);
int     big_add(uint8_t* a, uint_fast8_t len_a, uint8_t* b, uint_fast8_t len_b, uint8_t* out, uint_fast8_t max);
int     big_sub(uint8_t* a, uint_fast8_t len_a, uint8_t* b, uint_fast8_t len_b, uint8_t* out);
int     big_mul(uint8_t* a, uint_fast8_t la, uint8_t* b, uint_fast8_t lb, uint8_t* res, uint_fast8_t max);
int     big_div(uint8_t* a, uint_fast8_t la, uint8_t* b, uint_fast8_t lb, uint_fast8_t sig, uint8_t* res);
int     big_mod(uint8_t* a, uint_fast8_t la, uint8_t* b, uint_fast8_t lb, uint_fast8_t sig, uint8_t* res);
int     big_exp(uint8_t* a, uint_fast8_t la, uint8_t* b, uint_fast8_t lb, uint8_t* res);
int     big_log256(uint8_t* a, uint_fast8_t len);
#endif