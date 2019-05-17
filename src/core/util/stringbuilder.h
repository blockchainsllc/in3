/** @file 
 * simple string buffer used to dynamicly add content.
 * */

#ifndef __STR_BUILDER_H__
#define __STR_BUILDER_H__

#include "bytes.h"
#include <stdbool.h>
#include <stddef.h>

#define sb_add_hexuint(sb, i) sb_add_hexuint_l(sb, i, sizeof(i))

typedef struct sb {
  char*  data;
  size_t allocted;
  size_t len;
} sb_t;

sb_t* sb_new(char* chars);
sb_t* sb_init(sb_t* sb);
void  sb_free(sb_t* sb);

sb_t* sb_add_char(sb_t* sb, char c);
sb_t* sb_add_chars(sb_t* sb, char* chars);
sb_t* sb_add_range(sb_t* sb, char* chars, int start, int len);
sb_t* sb_add_key_value(sb_t* sb, char* key, char* value, int lv, bool as_string);
sb_t* sb_add_bytes(sb_t* sb, char* prefix, bytes_t* bytes, int len, bool as_array);
sb_t* sb_add_hexuint_l(sb_t* sb, uintmax_t uint, size_t l);

#endif
