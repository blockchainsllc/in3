#include <stdint.h>  
#include <stdbool.h>
#include <stdlib.h>

#ifndef BYTES_H
#define BYTES_H
#define ADDRESS(v,d) bytes_t v; uint8_t d[20];  v.data=d; v.len=20;
#define HASH(v,d) bytes_t v; uint8_t d[32];  v.data=d; v.len=32;
#define BYTES(v,d,len) bytes_t v; uint8_t d[len];  v.data=d; v.len=len;

/* a byte array */
typedef struct {
	int len;
	uint8_t *data;
} bytes_t;

typedef struct {
	int bsize;
	bytes_t b;
} bytes_builder_t;

/* allocates a new byte array with 0 filled */
bytes_t *b_new(char *data, int len);

/* printsa the bytes as hey to stdout */
void b_print(bytes_t *a);

/* compares 2 byte arrays and returns 1 for equal and 0 for not equal*/
int b_cmp(bytes_t *a, bytes_t *b);

/* frees the data */
void b_free(bytes_t *a);

/* clones a byte array*/
bytes_t* b_dup(bytes_t *a);


uint8_t b_read_byte(bytes_t* b, size_t* pos);
uint16_t b_read_short(bytes_t* b, size_t* pos);
uint32_t b_read_int(bytes_t* b, size_t* pos);
uint64_t b_read_long(bytes_t* b, size_t* pos);
char* b_read_chars(bytes_t* b, size_t* pos);
bytes_t* b_read_dyn_bytes(bytes_t* b, size_t* pos);
bytes_t* b_read_fixed_bytes(bytes_t* b, size_t* pos);

/* allocates a new byte array with 0 filled */
bytes_builder_t *bb_new();


void bb_write_chars(bytes_builder_t *bb,char* c, int len);
void bb_write_dyn_bytes(bytes_builder_t *bb, bytes_t* src);
void bb_write_fixed_bytes(bytes_builder_t *bb, bytes_t* src);
void bb_write_int(bytes_builder_t *bb, uint32_t val);
void bb_write_long(bytes_builder_t *bb, uint64_t val);
void bb_write_byte(bytes_builder_t *bb, uint8_t val);
void bb_write_short(bytes_builder_t *bb, uint16_t val);
bytes_t* bb_move_to_bytes(bytes_builder_t *bb);

#endif
