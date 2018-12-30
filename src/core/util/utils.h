

/** @file 
 * utility functions.
 * */

#ifndef UTILS_H
#define UTILS_H

#include "bytes.h"
#include "mem.h"
#include <stdint.h>

#define SWAP(a, b) \
  {                \
    void* p = a;   \
    a       = b;   \
    b       = p;   \
  }

typedef uint32_t      pb_size_t;
typedef uint_least8_t pb_byte_t;

/** converts the bytes to a unsigned long (at least the last max len bytes) */
uint64_t bytes_to_long(uint8_t* data, int len);

/** the number of bytes used for a conerting a hex into bytes. */
int size_of_bytes(int str_len);

/**  converts a hexchar to byte (4bit) */
uint8_t strtohex(char c);

/** convert a c string to a byte array storing it into a existing buffer */
int hex2byte_arr(char* buf, int len, uint8_t* out, int outbuf_size);

/** convert a c string to a byte array creating a new buffer */
bytes_t* hex2byte_new_bytes(char* buf, int len);

/** convefrts a bytes into hex */
void int8_to_char(uint8_t* buffer, int len, char* out);

/** hashes the bytes and creates a new bytes_t */
bytes_t* sha3(bytes_t* data);

/** writes 32 bytes to the pointer. */
int sha3_to(bytes_t* data, void* dst);

/** converts a long to 8 bytes */
void long_to_bytes(uint64_t val, uint8_t* dst);
/** converts a int to 4 bytes */
void int_to_bytes(uint32_t val, uint8_t* dst);
/** compares 32 bytes and returns 0 if equal*/
int hash_cmp(uint8_t* a, uint8_t* b);

/** duplicate the string */
char* _strdup(char* src, int len);

/** calculate the min number of byte to represents the len */
int min_bytes_len(uint64_t val);

#endif
