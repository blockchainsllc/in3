/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
 * 
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 * 
 * 
 * COMMERCIAL LICENSE USAGE
 * 
 * Licensees holding a valid commercial license may use this file in accordance 
 * with the commercial license agreement provided with the Software or, alternatively, 
 * in accordance with the terms contained in a written agreement between you and 
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
 * information please contact slock.it at in3@slock.it.
 * 	
 * Alternatively, this file may be used under the AGPL license as follows:
 *    
 * AGPL LICENSE USAGE
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/
#define _POSIX_C_SOURCE 199309L
#include "utils.h"
#include "../../third-party/crypto/sha3.h"
#include "bytes.h"
#include "debug.h"
#include "mem.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#endif

#ifndef __ZEPHYR__
#include <sys/time.h>
#else
#include <posix/sys/time.h>
#endif

#ifdef ESP_IDF
#include <unistd.h>
#endif

#ifdef __ZEPHYR__
static uint64_t time_zephyr(void* t) {
  UNUSED_VAR(t);
  return k_uptime_get();
}
static int rand_zephyr(void* s) {
  UNUSED_VAR(s);
  return (int) k_uptime_get();
}
static void srand_zephyr(unsigned int s) {
  return;
}
static time_func  in3_time_fn  = time_zephyr;
static rand_func  in3_rand_fn  = rand_zephyr;
static srand_func in3_srand_fn = srand_zephyr;
#else
/* __ZEPHYR__ */
static uint64_t time_libc(void* t) {
  UNUSED_VAR(t);
  return time(t);
}
static int rand_libc(void* s) {
  UNUSED_VAR(s);
  return rand();
}
static void srand_libc(unsigned int s) {
  return srand(s);
}
static time_func  in3_time_fn  = time_libc;
static rand_func  in3_rand_fn  = rand_libc;
static srand_func in3_srand_fn = srand_libc;
#endif /* __ZEPHYR__ */
#define MAX_UINT64 0xFFFFFFFFFFFFFFFF
void in3_sleep(uint32_t ms) {
#if defined(_WIN32) || defined(WIN32)
  Sleep(ms);
#elif defined(__ZEPHYR__)
  k_sleep(ms);
#elif defined(ESP_IDF)
  usleep(ms);
#elif defined(WASM)
  UNUSED_VAR(ms);
#else
  struct timespec ts;
  ts.tv_sec  = ms / 1000;             // whole seconds
  ts.tv_nsec = (ms % 1000000) * 1000; // remainder, in nanoseconds
  nanosleep(&ts, NULL);
#endif
}

void uint256_set(const uint8_t* src, wlen_t src_len, bytes32_t dst) {
  if (src_len < 32) memset(dst, 0, 32 - src_len);
  memcpy(dst + 32 - src_len, src, src_len);
}

void long_to_bytes(uint64_t val, uint8_t* dst) {
  for (int i = 7; i >= 0; i--, val >>= 8) dst[i] = val & 0xFF;
}

void int_to_bytes(uint32_t val, uint8_t* dst) {
  for (int i = 3; i >= 0; i--, val >>= 8) dst[i] = val & 0xFF;
}

uint8_t hexchar_to_int(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return 255;
}

#ifdef __ZEPHYR__

const char* u64_to_str(uint64_t value, char* buffer, int buffer_len) {
  // buffer has to be at least 21 bytes (max u64 val = 18446744073709551615 has 20 digits + '\0')
  if (buffer_len < 21) return "<ERR(u64_to_str): buffer too small>";

  buffer[buffer_len - 1] = '\0';
  int pos                = buffer_len - 1;
  do {
    buffer[--pos] = '0' + value % 10;
    value /= 10;
  } while (value > 0 && pos > 0);

  return &buffer[pos];
}
#endif

int hex_to_bytes(const char* buf, int len, uint8_t* out, int outbuf_size) {
  if (!buf || len < -1) return len == 0 ? 0 : -1;
  if (len == -1) len = strlen(buf);
  if (buf[0] == '0' && buf[1] == 'x') {
    buf += 2;
    len -= 2;
  }
  if (len == 0) return 0;

  int bytes_len = (len + 1) / 2, i = 0, j = 0;
  if (bytes_len > outbuf_size) return -1;
  if (len & 1) {
    // for a odd number of bytes, we use the first 4bit and then start with index 1
    out[0] = hexchar_to_int(buf[0]);
    j = i = 1;
  }

  for (; i < len; i += 2, ++j)
    out[j] = (hexchar_to_int(buf[i]) << 4) | hexchar_to_int(buf[i + 1]);

  return bytes_len;
}

bytes_t* hex_to_new_bytes(const char* buf, int len) {
  bytes_t* bytes = _malloc(sizeof(bytes_t));
  bytes->len     = (len + 1) / 2;
  bytes->data    = _malloc(bytes->len);
  hex_to_bytes(buf, len, bytes->data, bytes->len);
  return bytes;
}

int bytes_to_hex(const uint8_t* buffer, int len, char* out) {
  const char hex[] = "0123456789abcdef";
  int        i = 0, j = 0;
  while (j < len) {
    out[i++] = hex[(buffer[j] >> 4) & 0xF];
    out[i++] = hex[buffer[j] & 0xF];
    j++;
  }
  out[i] = '\0';
  return len * 2;
}

/** writes 32 bytes to the pointer. */
int keccak(bytes_t data, void* dst) {
  struct SHA3_CTX ctx;
  sha3_256_Init(&ctx);
  if (data.len) sha3_Update(&ctx, data.data, data.len);
  keccak_Final(&ctx, dst);
  return 0;
}

uint64_t bytes_to_long(const uint8_t* data, int len) {
  if (len > 8) {
    data += len - 8;
    len = 8;
  }
  uint64_t res = 0;
  int      i;
  for (i = 0; i < len; i++) {
    if (data[i])
      res |= ((uint64_t) data[i]) << (len - i - 1) * 8;
  }
  return res;
}

uint64_t char_to_long(const char* a, int l) {
  if (!a || l < -1) return MAX_UINT64;
  if (l == -1) l = strlen(a);
  if (a[0] == '0' && a[1] == 'x') { // it's a hex number
    long val = 0;
    for (int i = l - 1; i > 1; i--)
      val |= ((uint64_t) hexchar_to_int(a[i])) << (4 * (l - 1 - i));
    return val;
  }
  else if (l < 12) {
    char temp[12];
    strncpy(temp, a, l);
    temp[l] = 0;
    return atoi(temp);
  }
  return MAX_UINT64;
}

char* _strdupn(const char* src, int len) {
  if (len < 0) len = strlen(src);
  char* dst = _malloc(len + 1);
  strncpy(dst, src, len);
  dst[len] = 0;
  return dst;
}

int min_bytes_len(uint64_t val) {
  for (int i = 0; i < 8; i++, val >>= 8) {
    if (val == 0) return i;
  }
  return 8;
}

char* str_replace(char* orig, const char* rep, const char* with) {
  char* result;
  char* ins;
  char* tmp;
  int   len_rep;
  int   len_with;
  int   len_front;
  int   count;

  if (!orig || !rep)
    return NULL;
  len_rep = strlen(rep);
  if (len_rep == 0)
    return NULL;
  if (!with)
    with = "";
  len_with = strlen(with);

  ins = orig;
  for (count = 0; (tmp = strstr(ins, rep)); ++count) {
    ins = tmp + len_rep;
  }

  tmp = result = _malloc(strlen(orig) + (len_with - len_rep) * count + 1);
  while (count--) {
    ins       = strstr(orig, rep);
    len_front = ins - orig;
    tmp       = strncpy(tmp, orig, len_front) + len_front;
    tmp       = strcpy(tmp, with) + len_with;
    orig += len_front + len_rep;
  }
  strcpy(tmp, orig);
  return result;
}

char* str_replace_pos(char* orig, size_t pos, size_t len, const char* rep) {
  if (!orig) return NULL;

  size_t l = strlen(orig);
  if (pos > l) return NULL;

  char* tmp = _malloc(l - len + strlen(rep) + 1);
  if (tmp) {
    strncpy(tmp, orig, pos);
    tmp[pos] = '\0';
    if (rep) strcat(tmp, rep);
    strcat(tmp, orig + pos + len);
  }
  return tmp;
}

char* str_find(char* haystack, const char* needle) {
  if (haystack == NULL || needle == NULL)
    return NULL;

  for (; *haystack; haystack++) {
    const char *h, *n;
    for (h = haystack, n = needle; *h && *n && (*h == *n); ++h, ++n) {}
    if (*n == '\0')
      return haystack;
  }
  return NULL;
}

char* str_remove_html(char* data) {
  int len = strlen(data), i = 0, dst = 0, html = 0;
  for (; i < len; i++) {
    switch (data[i]) {
      case '<':
        html = true;
        break;
      case '>':
        html = false;
        break;
      case '\n':
      case '\t':
      case '\r':
      case ' ':
        if (dst && data[dst - 1] != ' ')
          data[dst++] = ' ';
        break;

      default:
        if (!html)
          data[dst++] = data[i];
    }
  }
  data[dst] = 0;
  return data;
}

uint64_t current_ms() {
#ifndef __ZEPHYR__
  struct timeval te;
  gettimeofday(&te, NULL);
  return te.tv_sec * 1000L + te.tv_usec / 1000;
#else
  return 1000L;
#endif
}

void     in3_set_func_time(time_func fn) { in3_time_fn = fn; }
uint64_t in3_time(void* t) { return in3_time_fn(t); }
void     in3_set_func_rand(rand_func fn) { in3_rand_fn = fn; }
int      in3_rand(void* s) { return in3_rand_fn(s); }
void     in3_set_func_srand(srand_func fn) { in3_srand_fn = fn; }
void     in3_srand(unsigned int s) { return in3_srand_fn(s); }

int64_t parse_float_val(const char* data, int32_t expo) {
  bool    neg = false, exp_found = false, frac = false;
  int64_t val   = 0;
  int     major = 0;
  for (const char* c = data; *c; c++) {
    switch (*c) {
      case '-':
        neg = true;
        break;
      case '.':
        frac = true;
        break;
      case 'e':
      case 'E':
        exp_found = true;
        break;
      default:
        if (exp_found)
          val = val * 10 + (*c - '0');
        else if (!frac)
          major++;
        break;
    }
  }

  if (exp_found)
    expo += (int32_t)(neg ? (0 - val) : val);
  neg = false;
  val = 0;
  expo += major;

  for (const char* c = data; expo > 0 && *c; c++) {
    if (*c == '-')
      neg = true;
    else if (*c == 'e' || *c == 'E')
      break;
    else if (*c != '.') {
      val = val * 10 + (*c - '0');
      expo--;
    }
  }

  if (neg) val = 0 - val;
  for (; expo > 0; expo--) val *= 10;
  return val;
}
