#include <stdint.h>  

#ifndef BYTES_H
#define BYTES_H

/* a byte array */
typedef struct {
	int len;
	uint8_t *data;
} bytes_t;

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

#endif
