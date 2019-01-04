/** @file 
 * util helper on byte arrays.
 * */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef BYTES_H
#define BYTES_H

/** a byte array */
typedef struct {
  uint32_t len;  /**< the length of the array ion bytes */
  uint8_t* data; /**< the byte-data  */
} bytes_t;

/** a byte-buffer to attach byte-functions. */
typedef struct {
  uint32_t bsize; /**< size of the currently allocated bytes */
  bytes_t  b;     /**< the bytes struct */
} bytes_builder_t;

/** allocates a new byte array with 0 filled */
bytes_t* b_new(char* data, int len);

/** printsa the bytes as hey to stdout */
void b_print(bytes_t* a);

/** compares 2 byte arrays and returns 1 for equal and 0 for not equal*/
int b_cmp(bytes_t* a, bytes_t* b);

/** frees the data */
void b_free(bytes_t* a);

/** clones a byte array*/
bytes_t* b_dup(bytes_t* a);

/** reads a byte on the current position and updates the pos afterwards. */
uint8_t b_read_byte(bytes_t* b, size_t* pos);
/** reads a short on the current position and updates the pos afterwards. */
uint16_t b_read_short(bytes_t* b, size_t* pos);
/** reads a integer on the current position and updates the pos afterwards. */
uint32_t b_read_int(bytes_t* b, size_t* pos);
/** reads a unsigned integer as bigendian on the current position and updates the pos afterwards. */
uint32_t b_read_int_be(bytes_t* b, size_t* pos, size_t len);
/** reads a long on the current position and updates the pos afterwards. */
uint64_t b_read_long(bytes_t* b, size_t* pos);
/** creates a new string (needs to be freed) on the current position and updates the pos afterwards. */
char* b_new_chars(bytes_t* b, size_t* pos);
/** reads bytesn (which have the length stored as prefix) on the current position and updates the pos afterwards. */
bytes_t* b_new_dyn_bytes(bytes_t* b, size_t* pos);
/** reads bytes with a fixed length on the current position and updates the pos afterwards. */
bytes_t* b_new_fixed_bytes(bytes_t* b, size_t* pos, int len);

/* creates a new bytes_builder */
bytes_builder_t* bb_new();
/** frees a bytebuilder and its content. */
void bb_free(bytes_builder_t* bb);

/** internal helper to increase the buffer if needed */
int bb_check_size(bytes_builder_t* bb, size_t len);

/** writes a string to the builder. */
void bb_write_chars(bytes_builder_t* bb, char* c, int len);
/** writes bytes to the builder with a prefixed length. */
void bb_write_dyn_bytes(bytes_builder_t* bb, bytes_t* src);
/** writes fixed bytes to the builder. */
void bb_write_fixed_bytes(bytes_builder_t* bb, bytes_t* src);
/** writes a ineteger to the builder. */
void bb_write_int(bytes_builder_t* bb, uint32_t val);
/** writes s long to the builder. */
void bb_write_long(bytes_builder_t* bb, uint64_t val);
/*! writes any integer value with the given length of bytes */
void bb_write_long_be(bytes_builder_t* bb, uint64_t val, int len);
/** writes a single byte to the builder. */
void bb_write_byte(bytes_builder_t* bb, uint8_t val);
/** writes a short to the builder. */
void bb_write_short(bytes_builder_t* bb, uint16_t val);
/** writes the bytes to the builder. */
void bb_write_raw_bytes(bytes_builder_t* bb, void* ptr, size_t len);
/** resets the content of the builder. */
void bb_clear(bytes_builder_t* bb);
/** replaces or deletes a part of the content. */
void bb_replace(bytes_builder_t* bb, int offset, int delete_len, uint8_t* data, int data_len);
/** frees the builder and moves the content in a newly created bytes struct (which needs to be freed later). */
bytes_t* bb_move_to_bytes(bytes_builder_t* bb);

// [len][data][len] [len][data][len] [len][data][len]
//                  ^
// [p1]
//
void bb_push(bytes_builder_t* bb, uint8_t* data, uint8_t len);
void bb_push(bytes_builder_t* bb, uint8_t* data, uint8_t len);

#endif
