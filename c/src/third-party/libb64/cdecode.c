/*
cdecoder.c - c source to a base64 decoding algorithm implementation

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

#include "cdecode.h"
#include <string.h>

int base64_decode_value(char value_in) {
  static const char decoding[]    = {62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -2, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};
  static const char decoding_size = sizeof(decoding);
  value_in -= 43;
  if (value_in > decoding_size) return -1;
  return decoding[(int) value_in];
}

void base64_init_decodestate(base64_decodestate* state_in) {
  state_in->step      = step_a;
  state_in->plainchar = 0;
}

int base64_decode_block(const char* code_in, const int length_in, char* plaintext_out, base64_decodestate* state_in) {
  const char* codechar  = code_in;
  char*       plainchar = plaintext_out;
  char        fragment;

  *plainchar = state_in->plainchar;

  switch (state_in->step) {
    while (1) {
      case step_a:
        if (codechar == code_in + length_in) {
          state_in->step      = step_a;
          state_in->plainchar = *plainchar;
          return plainchar - plaintext_out;
        }
        fragment   = (char) base64_decode_value(*codechar++);
        *plainchar = (fragment & 0x03f) << 2;
        // fall through
      case step_b:
        if (codechar == code_in + length_in) {
          state_in->step      = step_b;
          state_in->plainchar = *plainchar;
          return plainchar - plaintext_out;
        }
        fragment = (char) base64_decode_value(*codechar++);
        *plainchar++ |= (fragment & 0x030) >> 4;
        *plainchar = (fragment & 0x00f) << 4;
        // fall through
      case step_c:
        if (codechar == code_in + length_in) {
          state_in->step      = step_c;
          state_in->plainchar = *plainchar;
          return plainchar - plaintext_out;
        }
        fragment = (char) base64_decode_value(*codechar++);
        *plainchar++ |= (fragment & 0x03c) >> 2;
        *plainchar = (fragment & 0x003) << 6;
        // fall through
      case step_d:
        if (codechar == code_in + length_in) {
          state_in->step      = step_d;
          state_in->plainchar = *plainchar;
          return plainchar - plaintext_out;
        }
        fragment = (char) base64_decode_value(*codechar++);
        *plainchar++ |= (fragment & 0x03f);
    }
  }
  /* control should not reach here */
  return plainchar - plaintext_out;
}

size_t base64_decode_strlen(const char* ip) {
  const size_t lip = strlen(ip);
  size_t       lop = lip / 4 * 3;
  if (lip > 1 && ip[lip - 2] == '=' && ip[lip - 1] == '=')
    lop -= 2;
  else if (ip[lip - 1] == '=')
    lop -= 1;
  return lop;
}

static size_t base64_strlen_nopad(const char* ip) {
  size_t lip = strlen(ip);
  if (lip > 1 && ip[lip - 2] == '=' && ip[lip - 1] == '=')
    lip -= 2;
  else if (ip[lip - 1] == '=')
    lip -= 1;
  return lip;
}

uint8_t* base64_decode(const char* ip, size_t* len) {
  *len        = base64_decode_strlen(ip);
  uint8_t* op = malloc(*len + 1);
  if (op) {
    char*              c = (char*) op;
    base64_decodestate s;
    base64_init_decodestate(&s);
    int cnt = base64_decode_block(ip, base64_strlen_nopad(ip), c, &s);
    c += cnt;
  }
  return op;
}
