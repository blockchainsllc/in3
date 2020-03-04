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

#include "rlp.h"

static int ref(bytes_t* d, bytes_t* b, size_t l, uint8_t* s, int r) {
  d->len  = l;
  d->data = s;
  return (s >= b->data && (s + l) >= b->data && (s + l) <= (b->data + b->len)) ? r : -1;
}

void rlp_add_length(bytes_builder_t* bb, uint32_t len, uint8_t offset) {
  if (len < 56)
    bb_write_byte(bb, offset + len);
  else if (len < 0x100) { // l=1
    bb_write_byte(bb, offset + 55 + 1);
    bb_write_byte(bb, len);
  } else if (len < 0x10000) { // l=2
    bb_write_byte(bb, offset + 55 + 2);
    bb_write_long_be(bb, len, 2);
  } else if (len < 0x1000000) { // l==3
    bb_write_byte(bb, offset + 55 + 3);
    bb_write_long_be(bb, len, 3);
  } else { // l==4
    bb_write_byte(bb, offset + 55 + 4);
    bb_write_long_be(bb, len, 4);
  }
}

int rlp_decode(bytes_t* b, int index, bytes_t* dst) {
  size_t  p, i, l, n;
  uint8_t c;
  for (p = 0, i = 0; i < b->len; i++, p++) {
    c = b->data[i];
    if (c < 0x80) { // single byte-item
      if ((int) p == index) return ref(dst, b, 1, b->data + i, 1);
    } else if (c < 0xb8) { // 0-55 length-item
      if ((int) p == index) return ref(dst, b, c - 0x80, b->data + i + 1, 1);
      i += c - 0x80;
    } else if (c < 0xc0) { // very long item
      for (l = 0, n = 0; n < (uint8_t)(c - 0xB7); n++) l |= (*(b->data + i + 1 + n)) << (8 * ((c - 0xb7) - n - 1));
      if ((int) p == index) return ref(dst, b, l, b->data + i + c - 0xb7 + 1, 1);
      i += l + c - 0xb7;
    } else if (c < 0xf8) { // 0-55 byte long list
      l = c - 0xc0;
      if ((int) p == index) return ref(dst, b, l, b->data + i + 1, 2);
      i += l; // + 1;
    } else {  // very long list
      for (l = 0, n = 0; n < (uint8_t)(c - 0xF7); n++) l |= (*(b->data + i + 1 + n)) << (8 * ((c - 0xf7) - n - 1));
      if ((int) p == index) return ref(dst, b, l, b->data + i + c - 0xf7 + 1, 2);
      i += l + c - 0xf7;
    }
  }

  if (index < 0)
    return i == b->len ? (int) p : -3; /* error */
  else if (i > b->len)
    return -1; /* error */
  else
    return 0; /* data OK, but item at index doesn't exist */
}

int rlp_decode_in_list(bytes_t* b, int index, bytes_t* dst) {
  if (rlp_decode(b, 0, dst) != 2) return 0;
  return rlp_decode(dst, index, dst);
}

int rlp_decode_len(bytes_t* b) {
  return rlp_decode(b, -1, NULL);
}

void rlp_encode_item(bytes_builder_t* bb, bytes_t* val) {
  if (val->len == 1 && val->data[0] < 0x80) {
  } else if (val->len < 56)
    bb_write_byte(bb, val->len + 0x80);
  else
    rlp_add_length(bb, val->len, 0x80);
  bb_write_fixed_bytes(bb, val);
}

void rlp_encode_list(bytes_builder_t* bb, bytes_t* val) {
  rlp_add_length(bb, val->len, 0xc0);
  bb_write_fixed_bytes(bb, val);
}

bytes_builder_t* rlp_encode_to_list(bytes_builder_t* bb) {
  uint8_t         d[4];
  bytes_builder_t ll = {.bsize = 4, .b = {.len = 0, .data = (uint8_t*) &d}};
  rlp_add_length(&ll, bb->b.len, 0xc0);
  bb_replace(bb, 0, 0, d, ll.b.len);
  return bb;
}

bytes_builder_t* rlp_encode_to_item(bytes_builder_t* bb) {
  uint8_t         d[4];
  bytes_builder_t ll = {.bsize = 4, .b = {.len = 0, .data = (uint8_t*) &d}};

  if (bb->b.len == 1 && bb->b.data[0] < 0x80)
    return bb;
  else if (bb->b.len < 56)
    bb_write_byte(&ll, bb->b.len + 0x80);
  else
    rlp_add_length(&ll, bb->b.len, 0x80);
  bb_replace(bb, 0, 0, d, ll.b.len);
  return bb;
}
