#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//include <zephyr.h>

#include "bytes.h"
#include "utils.h"
#include "mem.h"

bytes_t *b_new(char *data, int len)
{
	bytes_t *b = _calloc(1, sizeof(bytes_t));

	b->len = len;
	b->data = _calloc(1, len);
	b->data = memcpy(b->data, data, len);

	return b;
}

void b_print(bytes_t *a)
{
	size_t i;
	if (!a)
		return;

	printf("Bytes: ");
	for (i = 0; i < a->len; i++)
		printf("%02x", a->data[i]);
	printf("\n");
}

int b_cmp(bytes_t *a, bytes_t *b)
{
	size_t i;

	if ((a && b) == 0)
		return 1;

	if (a->len!=b->len)
		return 0;

	for (i = 0; i < a->len; i++) {
		if (a->data[i] != b->data[i])
			return 0;
	}

	return 1;
}

void b_free(bytes_t *a)
{
	if (!a)
		return;

	_free(a->data);
	_free(a);
}

bytes_t *b_dup(bytes_t *a)
{
	bytes_t *out = _calloc(1, sizeof(bytes_t));

	out->data = _calloc(1, a->len);
	out->data = memcpy(out->data, a->data, a->len);
	out->len = a->len;

	return out;
}






uint8_t b_read_byte(bytes_t* b, size_t* pos) {
	uint8_t val = *(uint8_t*)(b->data + *pos);
	*pos+=1;
	return val;

}
uint16_t b_read_short(bytes_t* b, size_t* pos) {
	uint16_t val = *(uint16_t*)(b->data + *pos);
	*pos+=2;
	return val;
}
uint32_t b_read_int(bytes_t* b, size_t* pos) {
	uint32_t val = *(uint32_t*)(b->data + *pos);
	*pos+=4;
	return val;
}
uint64_t b_read_long(bytes_t* b, size_t* pos) {
	uint64_t val = *(uint64_t*)(b->data + *pos);
	*pos+=8;
	return val;
}
char* b_new_chars(bytes_t* b, size_t* pos) {
	size_t l = strlen((const char*) b->data+*pos);
	char* r = _malloc(l+1);
	memcpy(r,b->data+*pos,l+1);
	*pos+=l+1;
	return r;
}

uint32_t b_read_int_be(bytes_t* b, size_t* pos, size_t len) {
	uint32_t val=0;
	for (size_t i=0;i<len;i++) 
		val |= b->data[*pos + len -i-1] <<  (i*8);
	*pos+=len;
	return val;

}


bytes_t* b_new_dyn_bytes(bytes_t* b, size_t* pos) {
	size_t l = b_read_int(b,pos);
	bytes_t* r = _malloc(sizeof(bytes_t));
	r->data = _malloc(l);
	r->len = l;
	memcpy(r->data, b->data+*pos,l);
	*pos+=l;
	return r;
}
bytes_t* b_new_fixed_bytes(bytes_t* b, size_t* pos, int len) {
	bytes_t* r = _malloc(sizeof(bytes_t));
	r->data = _malloc(len);
    r->len = len;

	memcpy(r->data, b->data+*pos,len);
	*pos+=len;
	return r;
}

/* allocates a new byte array with 0 filled */
bytes_builder_t *bb_new() {
   bytes_builder_t* r = _malloc(sizeof(bytes_builder_t));
   r->b.data = _malloc(32);
   r->b.len  = 0;
   r->bsize  = 32;
   return r;
}

/* allocates a new byte array with 0 filled */
void bb_free(bytes_builder_t* bb) {
  _free(bb->b.data);
  _free(bb);
}

static void check_size(bytes_builder_t* bb, size_t len) {
    if (bb==NULL || len==0 || bb->b.len + len < bb->bsize) return;
	#ifdef ZEPHYR 
	size_t l= bb->bsize;  
	#endif
    while (bb->b.len + len >= bb->bsize)
       bb->bsize <<= 1;
	#ifdef ZEPHYR 
    bb->b.data = _realloc(bb->b.data, bb-> bsize,  l);
	#else 
    bb->b.data = _realloc(bb->b.data, bb-> bsize,  0);
	#endif
}
void bb_write_chars(bytes_builder_t *bb,char* c, int len) {
	check_size(bb,len+1);
	memcpy(bb->b.data+bb->b.len, c,len );
	bb->b.data[bb->b.len + len]=0;
    bb->b.len+=len+1;
}
void bb_write_dyn_bytes(bytes_builder_t *bb, bytes_t* src) {
	check_size(bb,src->len+4);
	*(uint32_t*) (bb->b.data+bb->b.len) = src->len;
	memcpy(bb->b.data+bb->b.len+4, src->data,src->len );
    bb->b.len+=src->len+4;
}
void bb_write_fixed_bytes(bytes_builder_t *bb, bytes_t* src) {
	check_size(bb,src->len);
	memcpy(bb->b.data+bb->b.len, src->data,src->len );
    bb->b.len+=src->len;
}
void bb_write_raw_bytes(bytes_builder_t *bb, void* ptr, size_t len) {
	check_size(bb,len);
	memcpy(bb->b.data+bb->b.len, ptr,len );
    bb->b.len+=len;
}
void bb_write_int(bytes_builder_t *bb, uint32_t val) {
	check_size(bb,4);
	*(uint32_t*) (bb->b.data+bb->b.len) = val;
    bb->b.len+=4;
}
void bb_write_long(bytes_builder_t *bb, uint64_t val) {
	check_size(bb,8);
	*(uint64_t*) (bb->b.data+bb->b.len) = val;
    bb->b.len+=8;
}
void bb_write_short(bytes_builder_t *bb, uint16_t val) {
	check_size(bb,2);
	*(uint16_t*) (bb->b.data+bb->b.len) = val;
    bb->b.len+=2;
}

void bb_write_long_be(bytes_builder_t *bb, uint64_t val, int len) {
	check_size(bb,len);
	int i,s=bb->b.len;
	for (i=len;i>0;i--) 
		bb->b.data[s+len-i]= (val >> (i*8-8)) & 0xFF;
    bb->b.len+=len;
}


void bb_write_byte(bytes_builder_t *bb, uint8_t val) {
	check_size(bb,1);
	*(uint8_t*) (bb->b.data+bb->b.len) = val;
    bb->b.len++;
}
bytes_t* bb_move_to_bytes(bytes_builder_t *bb) {
	bytes_t* b = _malloc(sizeof(bytes_t));
	b->len=bb->b.len;
	b->data=bb->b.data;
	_free(bb);
	return b;
}
void bb_clear(bytes_builder_t *bb) {
	bb->b.len=0;
}

void bb_replace(bytes_builder_t *bb, int offset, int delete_len, uint8_t* data, int data_len) {
	if (!delete_len && !data_len) return;
	check_size(bb, data_len-delete_len);
	memmove(bb->b.data+offset+data_len, bb->b.data+offset+delete_len, bb->b.len-offset-delete_len);
	if (data_len) 
	   memcpy(bb->b.data+offset,data,data_len);
	bb->b.len+=data_len-delete_len;
}

