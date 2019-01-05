#include "big.h"
#include <stdlib.h>
#include <string.h>
// +1 if a > b
// 0  if a==b
// -1 if a < b
int big_cmp(uint8_t* a, int len_a, uint8_t* b, int len_b) {
  int i;
  if (len_a == len_b) return memcmp(a, b, len_a);
  if (len_a > len_b) {
    for (i = 0; i < len_a - len_b; i++) {
      if (a[i] != 0) return 1;
    }
    return memcmp(a + len_a - len_b, b, len_b);
  }
  for (i = 0; i < len_b - len_a; i++) {
    if (b[i] != 0) return 1;
  }
  return memcmp(a, b + +len_b - len_a, len_a);
}

/**
 * returns 0 if the value is positive or 1 if negavtive. in this case the absolute value is copied to dst.
*/
int big_signed(uint8_t* val, int len, uint8_t* dst) {
  if ((*val & 128) == 0) return 0;
  if (len > 32) return -1;
  uint8_t  tmp[32];
  int      i;
  uint16_t rest = 1;
  memcpy(tmp, val, len);
  for (i = len - 1; i >= 0; i--) {
    rest += val[i] ^ 0XFF;
    tmp[i] = rest & 0xFF;
    rest >>= 8;
  }
  memcpy(dst, tmp, len);
  return 1;
}

void big_shift_left(uint8_t* a, int len, int bits) {
  uint8_t  i, r = bits % 8;
  uint16_t carry = 0;
  if ((r = bits % 8)) {
    for (i = len - 1; i >= 0; i--) {
      a[i] = (carry |= a[i] << r) & 0xFF;
      carry >>= 8;
    }
  }
  if ((r = (bits - r) >> 3)) {
    for (i = 0; i < len; i++)
      a[i] = i + r < len ? a[i + r] : 0;
  }
}

void big_shift_right(uint8_t* a, int len, int bits) {
  uint8_t  i, r = bits % 8;
  uint16_t carry = 0;
  if ((r = bits % 8)) {
    for (i = 0; i < len; i++) {
      a[i] = (carry |= (uint16_t) a[i] << (8 - r)) >> 8;
      carry <<= 8;
    }
  }
  if ((r = (bits - r) >> 3)) {
    for (i = len - 1; i >= 0; i--)
      a[i] = i - r >= 0 ? a[i - r] : 0;
  }
}

int big_int(uint8_t* val, int len) {
  switch (len) {
    case 1:
      return *val;
    case 2:
      return ((int32_t) val[0] << 8) + val[1];
    case 3:
      return ((int32_t) val[0] << 16) + ((int32_t) val[1] << 8) + val[2];
    case 4:
      return ((int32_t) val[0] << 24) + ((int32_t) val[1] << 16) + ((int32_t) val[2] << 8) + val[3];
    default:
      return -1;
  }
}
