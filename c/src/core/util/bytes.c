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

#include "bytes.h"
#include "log.h"
#include "mem.h"
#include "utils.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

bytes_t* b_new(const char* data, int len) {
  bytes_t* b = _calloc(1, sizeof(bytes_t));

  b->len  = len;
  b->data = _calloc(1, len);
  if (data)
    b->data = memcpy(b->data, data, len);

  return b;
}

void ba_print(const uint8_t* a, size_t l) {
  size_t i;
  if (!a) return;

  in3_log_disable_prefix();
  in3_log_trace(" 0x");
  for (i = 0; i < l; i++) in3_log_trace("%02x", a[i]);

  if (l < 9) {
    in3_log_trace(" ( %" PRId64 " ) ", bytes_to_long(a, l));
  }
  in3_log_enable_prefix();
}

void b_print(const bytes_t* a) {
  size_t i;
  if (!a) return;
#ifdef __ZEPHYR__
  printk("Bytes: ");
  for (i = 0; i < a->len; i++) printk("%02x", a->data[i]);
  printk("\n");
#else
  in3_log_disable_prefix();
  in3_log_trace("Bytes: ");
  for (i = 0; i < a->len; i++) in3_log_trace("%02x", a->data[i]);
  in3_log_trace("\n");
  in3_log_enable_prefix();
#endif
}

int b_cmp(const bytes_t* a, const bytes_t* b) {
  if ((a && b) == 0 || (a->len == 0 && b->len == 0)) return 1;
  return a->data && b->data && a->len == b->len && memcmp(a->data, b->data, a->len) == 0;
}

int bytes_cmp(bytes_t a, bytes_t b) {
  return b_cmp(&a, &b);
}

void b_free(bytes_t* a) {
  if (!a)
    return;

  _free(a->data);
  _free(a);
}

bytes_t* b_dup(const bytes_t* a) {
  if (a == NULL) return NULL;
  bytes_t* out = _calloc(1, sizeof(bytes_t));
  out->data    = _calloc(1, a->len);
  out->data    = memcpy(out->data, a->data, a->len);
  out->len     = a->len;
  return out;
}
bytes_t cloned_bytes(bytes_t data) {
  uint8_t* p = _malloc(data.len);
  memcpy(p, data.data, data.len);
  return (bytes_t){.data = p, .len = data.len};
}

uint8_t b_read_byte(bytes_t* b, size_t* pos) {
  uint8_t val = *(uint8_t*) (b->data + *pos);
  *pos += 1;
  return val;
}
uint32_t b_read_int(bytes_t* b, size_t* pos) {
  uint32_t val = (uint32_t) bytes_to_int(b->data + *pos, 4);
  *pos += 4;
  return val;
}
uint64_t b_read_long(bytes_t* b, size_t* pos) {
  uint64_t val = bytes_to_long(b->data + *pos, 8);
  *pos += 8;
  return val;
}
char* b_new_chars(bytes_t* b, size_t* pos) {
  size_t l = strlen((const char*) b->data + *pos);
  char*  r = _malloc(l + 1);
  memcpy(r, b->data + *pos, l + 1);
  *pos += l + 1;
  return r;
}

bytes_t* b_new_fixed_bytes(bytes_t* b, size_t* pos, int len) {
  bytes_t* r = _malloc(sizeof(bytes_t));
  r->data    = _malloc(len);
  r->len     = len;

  memcpy(r->data, b->data + *pos, len);
  *pos += len;
  return r;
}

/* allocates a new byte array with 0 filled */
bytes_builder_t* bb_newl(size_t l) {
  bytes_builder_t* r = _malloc(sizeof(bytes_builder_t));
  r->b.data          = _malloc(l);
  r->b.len           = 0;
  r->bsize           = l;
  return r;
}

/* allocates a new byte array with 0 filled */
void bb_free(bytes_builder_t* bb) {
  if (bb) _free(bb->b.data);
  _free(bb);
}

int bb_check_size(bytes_builder_t* bb, size_t len) {
  if (bb == NULL || len == 0 || bb->b.len + len < bb->bsize) return 0;
#ifdef __ZEPHYR__
  size_t l = bb->bsize;
#endif
  while (bb->b.len + len >= bb->bsize) bb->bsize <<= 1;
#ifdef __ZEPHYR__
  uint8_t* buffer = _realloc(bb->b.data, bb->bsize, l);
#else
  uint8_t* buffer = _realloc(bb->b.data, bb->bsize, 0);
#endif
  if (!buffer)
    return -1;
  else
    bb->b.data = buffer;

  return 0;
}
void bb_write_chars(bytes_builder_t* bb, char* c, int len) {
  bb_check_size(bb, len + 1);
  memcpy(bb->b.data + bb->b.len, c, len);
  bb->b.data[bb->b.len + len] = 0;
  bb->b.len += len + 1;
}

void bb_write_fixed_bytes(bytes_builder_t* bb, const bytes_t* src) {
  bb_check_size(bb, src->len);
  memcpy(bb->b.data + bb->b.len, src->data, src->len);
  bb->b.len += src->len;
}
void bb_write_raw_bytes(bytes_builder_t* bb, void* ptr, size_t len) {
  bb_check_size(bb, len);
  memcpy(bb->b.data + bb->b.len, ptr, len);
  bb->b.len += len;
}
void bb_write_int(bytes_builder_t* bb, uint32_t val) {
  bb_check_size(bb, 4);
  int_to_bytes(val, bb->b.data + bb->b.len);
  bb->b.len += 4;
}
void bb_write_long(bytes_builder_t* bb, uint64_t val) {
  bb_check_size(bb, 8);
  long_to_bytes(val, bb->b.data + bb->b.len);
  bb->b.len += 8;
}

void bb_write_long_be(bytes_builder_t* bb, uint64_t val, int len) {
  bb_check_size(bb, len);
  int i, s = bb->b.len;
  for (i = 0; i < len; i++) bb->b.data[s + len - i - 1] = (val >> (i << 3)) & 0xFF;
  bb->b.len += len;
}

void bb_write_byte(bytes_builder_t* bb, uint8_t val) {
  bb_check_size(bb, 1);
  *(uint8_t*) (bb->b.data + bb->b.len) = val;
  bb->b.len++;
}
bytes_t* bb_move_to_bytes(bytes_builder_t* bb) {
  bytes_t* b = _malloc(sizeof(bytes_t));
  b->len     = bb->b.len;
  b->data    = bb->b.data;
  _free(bb);
  return b;
}
void bb_clear(bytes_builder_t* bb) {
  bb->b.len = 0;
}

void bb_replace(bytes_builder_t* bb, int offset, int delete_len, uint8_t* data, int data_len) {
  if (!delete_len && !data_len) return;
  bb_check_size(bb, data_len - delete_len);
  memmove(bb->b.data + offset + data_len, bb->b.data + offset + delete_len, bb->b.len - offset - delete_len);
  if (data_len) memcpy(bb->b.data + offset, data, data_len);
  bb->b.len += data_len - delete_len;
}
