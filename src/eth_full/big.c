#include "big.h"
#include "util/utils.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static void optimze_length(uint8_t** data, uint8_t* l) {
  while (*l > 1 && **data == 0) {
    *l -= 1;
    *data += 1;
  }
}

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
  uint8_t  r     = bits % 8;
  uint16_t carry = 0;
  int      i;
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
  uint8_t  r     = bits % 8;
  uint16_t carry = 0;
  int      i;
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

int big_add(uint8_t* a, uint8_t len_a, uint8_t* b, uint8_t len_b, uint8_t* out, uint8_t max) {
  optimze_length(&a, &len_a);
  optimze_length(&b, &len_b);
  uint8_t  l     = len_a > len_b ? len_a + 1 : len_b + 1;
  uint16_t carry = 0;
  if (max && l > max) l = max;
  for (int i = l - 1;; i--) {
    carry |= (len_a ? a[--len_a] : 0) + (len_b ? b[--len_b] : 0);
    out[i] = carry & 0xFF;
    carry >>= 8;
    if (i == 0) break;
  }
  return l;
}

int big_sub(uint8_t* a, uint8_t len_a, uint8_t* b, uint8_t len_b, uint8_t* out) {
  optimze_length(&a, &len_a);
  optimze_length(&b, &len_b);
  uint8_t  l = len_a > len_b ? len_a + 1 : len_b + 1, borrow = 0;
  uint16_t carry = 0;
  if (l > 32) l = 32;
  for (int i = l - 1;; i--) {
    carry  = (uint16_t)(len_a ? a[--len_a] : 0) - (uint16_t)(len_b ? b[--len_b] : 0) - (uint16_t) borrow;
    out[i] = carry & 0xFF;
    borrow = (carry >> 8) & 1;
    if (i == 0) break;
  }
  if (borrow && l < 32) {
    // we need to fillfillout to word size
    memmove(out + 32 - l, out, l);
    memset(out, 0xFF, 32 - l);
    l = 32;
  }
  return l;
}

int big_mul(uint8_t* a, uint8_t la, uint8_t* b, uint8_t lb, uint8_t* res, uint8_t max) {
  uint16_t multiply_result16, partial_sum;
  uint8_t  v1, low_carry, high_carry, multiply_result_low8, multiply_result_high8, i, j, l = la + lb + 1, out[66], *r = out;

  memset(out, 0, la + lb + 1);
  for (i = la - 1; i >= 0 && i != 0xFF; i--) {
    v1         = a[i];
    high_carry = 0;
    for (j = lb; j >= 0 && j != 0xFF; j--) {
      // o = i + j
      multiply_result16     = (uint16_t) v1 * (uint16_t) b[j];
      multiply_result_low8  = multiply_result16 & 0xFF;
      multiply_result_high8 = multiply_result16 >> 8;
      partial_sum           = (uint16_t)((uint16_t) out[i + j + 1] + (uint16_t) multiply_result_low8);
      out[i + j + 1]        = (uint8_t) partial_sum;
      low_carry             = (uint8_t)(partial_sum >> 8);
      partial_sum           = (uint16_t)((uint16_t) out[i + j] + (uint16_t) multiply_result_high8 + (uint16_t) low_carry + (uint16_t) high_carry);
      out[i + j]            = (uint8_t) partial_sum;
      high_carry            = (uint8_t)(partial_sum >> 8);
    }
  }

  // optimize length by removeing leading zeros
  optimze_length(&r, &l);
  // copy result
  if (l <= max) {
    memcpy(res, r, l);
    return l;
  } else {
    // if too long we only copy the last 32 bytes which is alos the modulo of it.
    memcpy(res + l - max, r, max);
    return max;
  }
}

int big_div(uint8_t* a, uint8_t la, uint8_t* b, uint8_t lb, uint8_t sig, uint8_t* res) {
  optimze_length(&a, &la);
  optimze_length(&b, &lb);
  if (lb < 9 && la < 9) {
    uint64_t _a = bytes_to_long(a, la);
    uint64_t _b = bytes_to_long(b, lb);
    if (sig) {
      long_to_bytes(_b == 0 ? 0 : (uint64_t)((int64_t) _a / (int64_t) _b), res);
    } else {
      long_to_bytes(_b == 0 ? 0 : _a / _b, res);
    }
    return 8;
  }
  return -7; // notsupported yet
}

int big_mod(uint8_t* a, uint8_t la, uint8_t* b, uint8_t lb, uint8_t sig, uint8_t* res) {
  optimze_length(&a, &la);
  optimze_length(&b, &lb);
  if (lb > la) {
    // special case that the number is smaller than the modulo
    memcpy(a, res, la);
    return la;
  }
  if (lb < 9 && la < 9) {
    uint64_t _a = bytes_to_long(a, la);
    uint64_t _b = bytes_to_long(b, lb);
    if (sig) {
      long_to_bytes(_b == 0 ? 0 : (uint64_t)((int64_t) _a % (int64_t) _b), res);
    } else {
      long_to_bytes(_b == 0 ? 0 : _a % _b, res);
    }
    return 8;
  } else if (!sig) {
    // check if the mod is a power 2 value
    if ((*b & (*b - 1)) == 0) {
      for (int i = 1; i < lb; i++) {
        if (b[i] != 0) {
          // we can not do the shortcut, because it is not a power of 2 - number
          return -7; // not supported right now
        }
      }

      if (lb > 1) memcpy(res + 1, a + la - lb + 1, lb - 1);
      *res = *a & (*b - 1);
      return lb;
    }
  }
  return -7; // notsupported yet
}

int big_exp(uint8_t* a, uint8_t la, uint8_t* b, uint8_t lb, uint8_t sig, uint8_t* res) {
  optimze_length(&a, &la);
  optimze_length(&b, &lb);
  if (!sig && la == 1 && *a == 2) {
    memset(res, 0, 63);
    res[63] = 1;
    big_shift_left(res, 64, bytes_to_long(b, lb));
    return 64;
  }
  return -7;
}
