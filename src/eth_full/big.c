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
    if (b[i] != 0) return -1;
  }
  return memcmp(a, b + +len_b - len_a, len_a);
}

int big_sign(uint8_t* val, int len, uint8_t* dst) {
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
/**
 * returns 0 if the value is positive or 1 if negavtive. in this case the absolute value is copied to dst.
*/
int big_signed(uint8_t* val, int len, uint8_t* dst) {
  if ((*val & 128) == 0) return 0;
  if (len > 32) return -1;
  big_sign(val, len, dst);
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
    carry += (len_a ? a[--len_a] : 0) + (len_b ? b[--len_b] : 0);
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
  optimze_length(&a, &la);
  optimze_length(&b, &lb);
  if (la + lb < 9) {
    uint8_t rr[8], *p = rr, lr = 8;
    long_to_bytes(bytes_to_long(a, la) * bytes_to_long(b, lb), rr);
    optimze_length(&p, &lr);
    memcpy(res, p, lr);
    return lr;
  }

  uint8_t  out[66], i = la + lb, *p = out;
  uint32_t val = 0;
  int8_t   xs  = la - 1, ys, x, y;

  for (ys = lb - 1; ys >= 0; val >>= 8, ys--) {
    for (x = xs, y = ys; x >= 0 && y < lb; x--, y++) val += (uint32_t) a[x] * b[y];
    out[--i] = val & 0xFF;
  }

  for (ys = 0, xs = la - 2; xs >= 0; xs--, val >>= 8) {
    for (x = xs, y = ys; x >= 0 && y < lb; x--, y++) val += (uint32_t) a[x] * b[y];
    out[--i] = val & 0xFF;
  }
  out[--i] = val & 0xFF;
  i        = la + lb;
  optimze_length(&p, &i);
  if (i > max) {
    memcpy(res, p + i - max, max);
    i = max;
  } else
    memcpy(res, p, i);
  return i;
}

int big_bitlen(uint8_t* a, uint8_t la) {
  optimze_length(&a, &la);
  for (int i = 7; i >= 0; i--) {
    if (*a & (1 << i)) return la * 8 - 7 + i;
  }
  return la * 8 - 8;
}

int big_exp(uint8_t* a, uint8_t la, uint8_t* b, uint8_t lb, uint8_t* res) {
  optimze_length(&a, &la);
  optimze_length(&b, &lb);
  if (lb == 0 || (lb == 1 && *b == 0)) {
    res[0] = 1;
    return 1;
  }
  if (lb == 1 && *b == 1) {
    memcpy(res, a, la);
    return la;
  }
  if (la == 1 && *a == 2) {
    if (lb > 1) {
      *res = 0;
      return 1;
    }

    memset(res, 0, 63);
    res[63]      = 1;
    uint32_t exp = bytes_to_long(b, lb);
    if (exp > 255) {
      *res = 0;
      return 1;
    }
    big_shift_left(res, 64, exp);
    uint8_t *p = res, l = 64;
    optimze_length(&p, &l);
    if (l > 32) {
      p += l - 32;
      l = 32;
    }
    if (p != res) memmove(res, p, l);
    return l;
  } else {
    uint8_t wnd[16 * 32], tmp[65], r[65], word, first = 1, *p = r;
    int     i, j, l, current = 0, current_len = 0, start = big_bitlen(b, lb) % 8, rl = 32;
    memset(wnd, 0, 16 * 32);
    wnd[31] = 1;
    memcpy(wnd + 32 + 32 - la, a, la);
    for (i = 2; i < 16; i++) {
      l = big_mul(wnd + (i - 1) * 32, 32, a, la, tmp, 32);
      memcpy(wnd + i * 32 + 32 - l, tmp, l);
    }
    memcpy(r, wnd, 32);

    if (start == 0) start = 8;
    for (i = lb - 1; i >= 0; i--) {
      word = b[i];
      for (j = start - 1; j >= 0; j--) {
        uint8_t bit = (word >> j) & 1;
        if (!first) {
          memcpy(tmp, r, rl);
          rl = big_mul(tmp, rl, tmp, rl, r, 32);
        }

        if (bit == 0 && current == 0) {
          current_len = 0;
          continue;
        }

        current <<= 1;
        current |= bit;
        current_len++;
        if (current_len != 4 && (i != 0 || j != 0)) continue;

        first = 0;
        memcpy(tmp, r, rl);
        rl          = big_mul(tmp, rl, wnd + 32 * current, 32, r, 32);
        current_len = 0;
        current     = 0;
      }
      start = 8;
    }

    first = rl;
    optimze_length(&p, &first);

    memcpy(res, p, first);
    return first;
  }

  // a ** num

  /*
      if (num.isZero()) return new BN(1).toRed(this);
      if (num.cmpn(1) === 0) return a.clone();
  
      var windowSize = 4;
      var wnd = new Array(1 << windowSize);
      wnd[0] = new BN(1).toRed(this);
      wnd[1] = a;
      for (var i = 2; i < wnd.length; i++) {
        wnd[i] = this.mul(wnd[i - 1], a);
      }


  
      var res = wnd[0];
      var current = 0;
      var currentLen = 0;
      var start = num.bitLength() % 26;
      if (start === 0) {
        start = 26;
      }
  
      for (i = num.length - 1; i >= 0; i--) {
        var word = num.words[i];
        for (var j = start - 1; j >= 0; j--) {
          var bit = (word >> j) & 1;
          if (res !== wnd[0]) {
            res = this.sqr(res);
          }
  
          if (bit === 0 && current === 0) {
            currentLen = 0;
            continue;
          }
  
          current <<= 1;
          current |= bit;
          currentLen++;
          if (currentLen !== windowSize && (i !== 0 || j !== 0)) continue;
  
          res = this.mul(res, wnd[current]);
          currentLen = 0;
          current = 0;
        }
        start = 26;
      }
  
      return res;
  */
}

int big_log256(uint8_t* a, int len) {
  while (a[0] == 0) {
    len--;
    a++;
  }
  return len;
}

int big_divmod(uint8_t* n, uint8_t ln, uint8_t* d, uint8_t ld, uint8_t* q, int* qlen, uint8_t* remain, int* remain_len) {
  int     j, i;
  uint8_t l = 8;

  optimze_length(&n, &ln);
  optimze_length(&d, &ld);

  if (ld < 8) {
    // we can use long here
    uint64_t ur = 0, ud = bytes_to_long(d, ld);
    uint8_t  pp[8], *p  = pp;

    if (ln < 9) {
      // shortcurt for pure long operation
      ur = bytes_to_long(n, ln);
      long_to_bytes(ur / ud, p);
      optimze_length(&p, &l);
      memcpy(q, p, l);
      *qlen = l;
      l     = 8;
      p     = pp;
      long_to_bytes(ur % ud, p);
      optimze_length(&p, &l);
      if (remain) {
        memcpy(remain, p, l);
        *remain_len = l;
      }
      return 0;
    }

    // iterate over n with divisor as long
    for (i = 0, j = -1; i < ln; i++) {
      ur = (ur << 8) | n[i];
      if (ur < ud) {
        if (j >= 0) q[++j] = 0;
      } else {
        q[++j] = ur / ud;
        ur     = ur % ud;
      }
    }

    *qlen = j + 1;
    long_to_bytes(ur, p);
    optimze_length(&p, &l);
    if (remain) {
      memcpy(remain, p, l);
      *remain_len = l;
    }
    return 0;
  } else {

    uint8_t row[32], tmp[33], tmp2[33], val, min, max;
    int     res;
    memset(row, 0, 32);

    for (i = 0, j = -1; i < ln; i++) {
      memmove(row, row + 1, 31);
      row[31] = n[i];
      if (big_cmp(row, 32, d, ld) < 0) {
        if (j >= 0) q[++j] = 0;
      } else {
        val = 128;
        min = 1;
        max = 255;
        while (true) {
          l   = big_mul(&val, 1, d, ld, tmp, 33);
          res = big_cmp(row, 32, tmp, l);
          if (res < 0) {
            max = val;
            val = min + (max - min) / 2;
            if (val == min) break;
          } else if (res > 0) {
            min = val;
            val = min + (max - min) / 2;
            if (val == min) val++;
          } else
            break;
        }
        q[++j] = val;
        if (res) {
          l = big_mul(&val, 1, d, ld, tmp, 33);
          l = big_sub(row, 32, tmp, l, tmp2);
          memset(row, 0, 32);
          memcpy(row + 32 - l, tmp2, l);
        } else
          memset(row, 0, 32);
      }
    }

    uint8_t *p = row, pl = 32;
    optimze_length(&p, &pl);
    if (remain) {
      memcpy(remain, p, pl);
      *remain_len = pl;
    }
    *qlen = j + 1;
  }
  return 0;
}

int big_div(uint8_t* a, uint8_t la, uint8_t* b, uint8_t lb, uint8_t sig, uint8_t* res) {
  int l;

  optimze_length(&a, &la);
  optimze_length(&b, &lb);

  while (la > 1 && lb > 1 && a[la - 1] == 0 && b[lb - 1] == 0) {
    la--;
    lb--;
  }

  if (lb == 0 || (lb == 1 && b[0] == 0)) {
    *res = 0;
    return 1;
  }

  if (sig) {
    uint8_t _a[32], _b[32], sa, sb;
    sa = big_signed(a, la, _a);
    sb = big_signed(b, lb, _b);
    big_divmod(sa ? _a : a, la, sb ? _b : b, lb, res, &l, NULL, NULL);

    if (sa != sb)
      big_sign(res, l, res);
    return l;
  }

  big_divmod(a, la, b, lb, res, &l, NULL, 0);
  return l;
}

int big_mod(uint8_t* a, uint8_t la, uint8_t* b, uint8_t lb, uint8_t sig, uint8_t* res) {
  int     l, l2;
  uint8_t tmp[65];

  optimze_length(&a, &la);
  optimze_length(&b, &lb);

  if (lb > la) {
    // special case that the number is smaller than the modulo
    memcpy(res, a, la);
    return la;
  }

  if (lb == 0 || (lb == 1 && b[0] == 0)) {
    *res = 0;
    return 1;
  }

  if (!sig) {
    l = 1;
    // check if the mod is a power 2 value
    if ((*b & (*b - 1)) == 0) {
      for (int i = 1; i < lb; i++) {
        if (b[i] != 0) {
          // we can not do the shortcut, because it is not a power of 2 - number
          l = 0;
          break;
        }
      }

      if (l) {
        if (lb > 1) memcpy(res + 1, a + la - lb + 1, lb - 1);
        *res = *a & (*b - 1);
        return lb;
      }
    }
  } else {
    uint8_t _a[32], _b[32], sa, sb;
    sa = big_signed(a, la, _a);
    sb = big_signed(b, lb, _b);
    big_divmod(sa ? _a : a, la, sb ? _b : b, lb, tmp, &l2, res, &l);

    if (sa != sb)
      big_sign(res, l, res);
    return l;
  }

  int rres = big_divmod(a, la, b, lb, tmp, &l2, res, &l);
  return rres < 0 ? rres : l;
}
