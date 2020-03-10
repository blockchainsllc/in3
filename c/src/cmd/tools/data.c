/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
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

/** @file 
 * simple commandline-util parsing json and creating bin
 * */

#include "../../core/util/data.h"
#include "../../core/util/colors.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "used_keys.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void add_keyname(const char* name, d_key_t value, size_t len);

bytes_t read_from_stdin(FILE* file) {
  if (file == NULL) {
    printf("File not found!");
    _Exit(1);
    return bytes(NULL, 0);
  }

  size_t   allocated = 1024;
  size_t   len       = 0;
  uint8_t* buffer    = _malloc(1025);
  size_t   r;

  while (1) {
    r = fread(buffer + len, 1, allocated - len, file);
    len += r;
    if (feof(file)) break;
    size_t new_alloc = allocated * 2 + 1;
    buffer           = _realloc(buffer, new_alloc, allocated);
    allocated        = new_alloc;
  }

  buffer[len] = 0;
  if (file != stdin) fclose(file);
  return bytes(buffer, len);
}

#define C_RED "0;31"
#define C_GREEN "0;32"
#define C_ORANGE "0;33"
#define C_BLUE "0;34"
#define C_PURPLE "0;35"
#define C_CYAN "0;36"
#define C_LGRAY "0;37"
#define C_DGRAY "1;30"
#define C_LRED "1;31"
#define C_LGREEN "1;32"
#define C_YELLOW "1;33"
#define C_LBLUE "1;34"
#define C_LPURPLE "1;35"
#define C_LCYAN "1;36"

static inline d_key_t keyhash(const char* c) {
  uint16_t val = 0;
  size_t   l   = strlen(c);
  for (; l; l--, c++) val ^= *c | val << 7;
  return val;
}

static void init_keys() {
  for (int i = 0; USED_KEYS[i]; i++)
    add_keyname(USED_KEYS[i], keyhash(USED_KEYS[i]), strlen(USED_KEYS[i]));
}

static void print_hex(uint8_t* d, uint32_t l, char* color) {
  if (color) printf(COLORT_SELECT, color);
  for (uint32_t i = 0; i < l; i++)
    printf("%02x", d[i]);
  printf(" ");
  if (color) printf(COLORT_RESET);
}

static int read_token(uint8_t* d, size_t* p, int level, int* index, int keyval) {
  printf("%03i: " COLORT_RRED, *index);
  d_type_t type = d[*p] >> 5;
  uint32_t len  = d[*p] & 0x1F, i; // the other 5 bits  (0-31) the length
                                   // first 3 bits define the type
  switch (type) {
    case T_ARRAY:
      printf("<ARRAY> ");
      break;
    case T_OBJECT:
      printf("<OBJECT>");
      break;
    case T_STRING:
      printf("<STRING>");
      break;
    case T_BYTES:
      printf("<BYTES> ");
      break;
    case T_INTEGER:
      printf("<INT>   ");
      break;
    case T_BOOLEAN:
      if (len)
        printf("<REF>   ");
      else
        printf("<BOOL>  ");
      break;
    case T_NULL:
      if (len)
        printf("<INIT>  ");
      else
        printf("<NULL>  ");
      break;
  }
  printf(" ");

  for (int i = 0; i < level; i++) printf(COLORT_BBLACK "." COLORT_RESET);
  if (keyval >= 0) {
    char* keyname = d_get_keystr((d_key_t) keyval);
    if (keyname)
      printf(COLORT_RMAGENTA "%s" COLORT_RESET, keyname);
    else {
      uint8_t tmp[2];
      tmp[0] = (keyval >> 8) & 0xFF;
      tmp[1] = keyval & 0xFF;
      print_hex(tmp, 2, C_PURPLE);
    }
  }

  uint16_t key;
  print_hex(d + (*p), 1, C_GREEN);

  // calculate len
  (*p)++;
  int l = len > 27 ? len - 27 : 0;
  if (len == 28)
    len = d[*p]; // 28 = 1 byte len
  else if (len == 29)
    len = d[*p] << 8 | d[*p + 1]; // 29 = 2 bytes length
  else if (len == 30)
    len = d[*p] << 16 | d[*p + 1] << 8 | d[*p + 2]; // 30 = 3 bytes length
  else if (len == 31)
    len = d[*p] << 24 | d[*p + 1] << 16 | d[*p + 2] << 8 | d[*p + 3]; // 31 = 4 bytes length

  print_hex(d + (*p) + 1, l, C_ORANGE);
  *p += l;

  // special token giving the number of tokens, so we can allocate the exact number
  if (type == T_NULL && len > 0) {
    printf("  len = %i\n", len);
    read_token(d, p, level, index, -1);
    return 0;
  }
  *index = *index + 1;

  // special handling for references
  if (type == T_BOOLEAN && len > 1) {
    printf("   idx = %i\n", len - 2);
    return 0;
  }

  switch (type) {
    case T_ARRAY:
      printf("  len = %i\n", len);
      for (i = 0; i < len; i++) {
        if (read_token(d, p, level + 1, index, -1)) return 1;
      }
      break;
    case T_OBJECT:
      printf("  len = %i\n", len);
      for (i = 0; i < len; i++) {
        key = d[(*p)] << 8 | d[*p + 1];
        *p += 2;
        if (read_token(d, p, level + 1, index, key)) return 1;
      }
      break;
    case T_STRING:
      print_hex(d + (*p), len + 1, C_LGRAY);
      printf("  len = %i val=\"%s\"\n", len, d + ((*p)++));
      *p += len;
      break;
    case T_BYTES:
      print_hex(d + (*p), len, C_LGRAY);
      printf("  len = %i\n", len);
      *p += len;
      break;
    case T_INTEGER:
      printf("  val = %i\n", len);
      break;
    case T_BOOLEAN:
      printf("  val = %i\n", len);
      break;
    case T_NULL:
      printf("  \n");
      break;
    default:
      break;
  }
  return 0;
}

static void print_debug(bytes_t* data) {
  size_t p     = 0;
  int    index = 0;
  read_token(data->data, &p, 0, &index, -1);
}

int main(int argc, char* argv[]) {
  bytes_t input  = bytes(NULL, 0);
  char*   format = "auto";
  bool    debug  = false;

  int i;
  init_keys();
  // fill from args
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-f") == 0)
      input = read_from_stdin(fopen(argv[++i], "r"));
    else if (strcmp(argv[i], "-o") == 0)
      format = argv[++i];
    else if (strcmp(argv[i], "-d") == 0)
      debug = true;
    else {
      input.data = (uint8_t*) argv[i];
      input.len  = strlen(argv[i]);
    }
  }

  if (input.data == NULL) input = read_from_stdin(stdin);

  if (!input.len) return 0;

  if (!strcmp(format, "auto")) {
    if (input.data[0] == '0' && input.data[1] == 'x') {
      input.data = malloc(strlen(argv[i]) / 2);
      input.len  = hex_to_bytes(argv[i], -1, input.data, strlen(argv[i]) / 2);
      format     = "json";
    } else
      format = "hex";
  }

  if (!strcmp(format, "json")) {
    if (debug) {
      print_debug(&input);
      return 0;
    }
    json_ctx_t* ctx = parse_binary(&input);
    if (!ctx) {
      printf("Invalid binary data!\n");
      return 1;
    }
    printf("%s\n", d_create_json(ctx->result));
    return 0;
  }

  json_ctx_t* ctx = parse_json((char*) input.data);
  if (!ctx)
    printf("Invalid Json : %s\n", (char*) input.data);
  else {
    bytes_builder_t* bb = bb_new();
    d_serialize_binary(bb, ctx->result);

    if (strcmp(format, "hex") == 0) {
      for (i = 0; i < (int) bb->b.len; i++) printf("%02x", bb->b.data[i]);
      printf("\n");
    } else if (strcmp(format, "cstr") == 0) {
      unsigned char c = 0, is_hex = 0;

      for (i = 0; i < (int) bb->b.len; i++) {
        c      = bb->b.data[i];
        is_hex = c < ' ' || c > 0x7E || c == 0x5C || c == '"' || (is_hex && ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')));
        printf(is_hex ? "\\x%02x" : "%c", c);
      }
      printf("\n len = %u\n", bb->b.len);
    } else if (strcmp(format, "bin") == 0) {
      for (uint32_t i = 0; i < bb->b.len; i++)
        putchar(bb->b.data[i]);

    } else {
      printf("unsuported output format %s!\n", format);
      return 1;
    }
  }

  return 0;
}