#include "bytes.h"
#include "log.h"
#include "mem.h"
#include "utils.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

bytes_t* b_new(char* data, int len) {
  bytes_t* b = _calloc(1, sizeof(bytes_t));

  b->len  = len;
  b->data = _calloc(1, len);
  b->data = memcpy(b->data, data, len);

  return b;
}

void ba_print(uint8_t* a, size_t l) {
  size_t i;
  if (!a) return;

  in3_log_trace(" 0x");
  for (i = 0; i < l; i++) in3_log_trace("%02x", a[i]);

  if (l < 9) {
    in3_log_trace(" ( %" PRId64 " ) ", bytes_to_long(a, l));
  }
}

void b_print(bytes_t* a) {
  size_t i;
  if (!a) return;
#ifdef __ZEPHYR__
  printk("Bytes: ");
  for (i = 0; i < a->len; i++) printk("%02x", a->data[i]);
  printk("\n");
#else
  in3_log_trace("Bytes: ");
  for (i = 0; i < a->len; i++) in3_log_trace("%02x", a->data[i]);
  in3_log_trace("\n");
#endif
}

int b_cmp(bytes_t* a, bytes_t* b) {
  if ((a && b) == 0) return 1;

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

bytes_t* b_dup(bytes_t* a) {
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
uint16_t b_read_short(bytes_t* b, size_t* pos) {
  uint16_t val = *(uint16_t*) (b->data + *pos);
  *pos += 2;
  return val;
}
uint32_t b_read_int(bytes_t* b, size_t* pos) {
  uint32_t val = *(uint32_t*) (b->data + *pos);
  *pos += 4;
  return val;
}
uint64_t b_read_long(bytes_t* b, size_t* pos) {
  uint64_t val = *(uint64_t*) (b->data + *pos);
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

uint32_t b_read_int_be(bytes_t* b, size_t* pos, size_t len) {
  uint32_t val = 0;
  for (size_t i = 0; i < len; i++) val |= b->data[*pos + len - i - 1] << (i * 8);
  *pos += len;
  return val;
}

bytes_t* b_new_dyn_bytes(bytes_t* b, size_t* pos) {
  size_t   l = b_read_int(b, pos);
  bytes_t* r = _malloc(sizeof(bytes_t));
  r->data    = _malloc(l);
  r->len     = l;
  memcpy(r->data, b->data + *pos, l);
  *pos += l;
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
void bb_write_dyn_bytes(bytes_builder_t* bb, bytes_t* src) {
  bb_check_size(bb, src->len + 4);
  *(uint32_t*) (bb->b.data + bb->b.len) = src->len;
  memcpy(bb->b.data + bb->b.len + 4, src->data, src->len);
  bb->b.len += src->len + 4;
}
void bb_write_fixed_bytes(bytes_builder_t* bb, bytes_t* src) {
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
  *(uint32_t*) (bb->b.data + bb->b.len) = val;
  bb->b.len += 4;
}
void bb_write_long(bytes_builder_t* bb, uint64_t val) {
  bb_check_size(bb, 8);
  *(uint64_t*) (bb->b.data + bb->b.len) = val;
  bb->b.len += 8;
}
void bb_write_short(bytes_builder_t* bb, uint16_t val) {
  bb_check_size(bb, 2);
  *(uint16_t*) (bb->b.data + bb->b.len) = val;
  bb->b.len += 2;
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
