

/** @file 
 * utility functions.
 * */ 

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "bytes.h"
#include "mem.h"

#define SWAP(a,b) { void* p=a; a=b; b=p; }

typedef uint32_t pb_size_t;
typedef uint_least8_t pb_byte_t;

#define CLONE(str) char* =     bytes_t v; uint8_t d[20];  v.data=d; v.len=20;


#define JSON_OBJECT(ob, str, tokens) json_object_t ob = { .js=str, .tok=tokens };


uint64_t bytes_to_long (uint8_t* data, int len);

int size_of_bytes(int str_len);
uint8_t strtohex(char c);
int hex2byte_arr(char *buf, int len, uint8_t *out, int outbuf_size);
bytes_t* hex2byte_new_bytes(char *buf, int len);
void int8_to_char(uint8_t *buffer, int len, char *out);

bytes_t *sha3(bytes_t *data);
/**
 * writes 32 bytes to the pointer.
 */
int sha3_to(bytes_t* data, void*dst);


void long_to_bytes(uint64_t val, uint8_t* dst);
void int_to_bytes(uint32_t val, uint8_t* dst);
void byte_to_hex(uint8_t b, char s[23]);
int hash_cmp(uint8_t *a, uint8_t *b);


char* _strdup(char* src, int len);
int str2hex_str(char *str, char **buf);
int str2byte_a(char *str, uint8_t **buf);

#define EQ_MODE_CASE_INSENSITIVE  1
#define EQ_MODE_CASE_NUMBER  2

int min_bytes_len(uint64_t val);
long c_to_int(char* a, int l) ;
char c_to_lower(char c);

bool equals_range(char* a, int la,char* b, int lb, uint8_t mode) ;

#endif
