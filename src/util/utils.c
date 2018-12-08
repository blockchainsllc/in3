#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../jsmn/jsmnutil.h"
#include "bytes.h"

#include "debug.h"
#include "../crypto/sha3.h"

void byte_to_hex(uint8_t b, char s[23]) {
	unsigned i=1;
	s[0] = s[1] = '0';
	s[2] = '\0';
	while(b) {
		unsigned t = b & 0x0f;
		if( t < 10 ) {
			s[i] = '0' + t;
		} else {
			s[i] = 'a' + t - 10;
		}
		i--;
		b >>= 4;
	}
}

void *k_realloc(void *ptr, size_t size, size_t oldsize)
{
	void *new;

	new = malloc(size);
	if (!new)
		goto error;

	if (ptr && oldsize) {
		memcpy(new, ptr, oldsize);
		free(ptr);
	}

	return new;

error:
	return NULL;
}

int json_get_int_value(char *data, char *key)
{
	int tokc, value;
	jsmntok_t *tokv;

	jsmnutil_parse_json(data, &tokv, &tokc);
	value = get_json_key_value_int(data, key, tokv, tokc);

	if (tokv)
		free(tokv);

	return value;
}

char *json_get_value(char *data, char *key)
{
	int tokc;
	char *value;
	jsmntok_t *tokv;

	jsmnutil_parse_json(data, &tokv, &tokc);
	value = get_json_key_value(data, key, tokv, tokc);

	if (tokv)
		free(tokv);

	return value;
}

int hash_cmp(uint8_t *a, uint8_t *b)
{
	int len = 31;

	while (a[len] == b[len] && len--);

	return ++len;
}

int str2hex_str(char *str, char **buf)
{
	int len, i = 0;
	char c[3];

	if (!str)
		return 0;

	len = strlen(str) * 2;

	*buf = calloc(1, (len + 3) * sizeof(char));
	if (*buf == 0)
		return 0;

	(*buf)[i++] = '0';
	(*buf)[i++] = 'x';

	while (*str) {
		sprintf(c, "%02x", (unsigned char) *str++);
		(*buf)[i++] = c[0];
		(*buf)[i++] = c[1];
	}

	return len;
}

int str2byte_a(char *str, uint8_t **buf)
{
	unsigned int off = 0, size = 0, nbytes = 0;

	if (!str)
		return 0;

	if (!strncmp("0x", str, 2))
		off = 2;

	size = strlen(str) - off;
	nbytes = size_of_bytes(size);

	*buf = malloc(nbytes);
	if (*buf == 0)
		return 0;

	if (hex2byte_arr(str+off, size, *buf, nbytes))
		return nbytes;

	free(*buf);
	return 0;
}

int get_json_key_value_int(char *buf, char *key, jsmntok_t* tok, int tokc)
{
	int i;
	int val = 0;
	int t=-1;

	for(i=0; i<tokc; i++) {
		int n = tok[i].end - tok[i].start;
		if (tok[i+1].type == JSMN_PRIMITIVE
		    && !strncmp(buf + tok[i].start, key, n)) {
			t=1;
		} else if (t==1) {
			char *idval = malloc(n+1);
			idval[n] = 0;
			strncpy(idval, buf + tok[i].start, n);
			val = atoi(idval);
			free(idval);
			return val;
		} else if (t==1) {
			return val;
		}
	}
	return val;
}

char* get_json_key_value(char *buf, char *key, jsmntok_t* tok, int tokc)
{
	int i;
	int t=-1;
	int is_key;
	unsigned short idx = 0;

	for(i=0; i<tokc; i++) {
		int n = tok[i].end - tok[i].start;
		if (t==-1 && tok[i].type == JSMN_OBJECT && n > 2) {
			idx = 1 - idx;
			continue;
		}
		is_key = idx ? (i & 1) :(i & ~1);
		if (n && (strlen(key) == n) && tok[i].type == JSMN_STRING
		    && !strncmp(buf + tok[i].start, key, n) && is_key) {
			t=1;
		} else if (t==1) {
			char *idval = malloc(n+1);
			idval[n] = 0;
			strncpy(idval, buf + tok[i].start, n);
			return idval;
		} else if (t==1) {
			return NULL;
		}
	}
	return NULL;
}

int json_get_token_size(jsmntok_t* t) {
	int i,j=1;
	switch (t->type) {
		case JSMN_PRIMITIVE:
		case JSMN_STRING:
		  return 1;
		case JSMN_OBJECT:
		  for (i=0;i<t->size;i++) 
			  j+=1+json_get_token_size(t+j+1);
		  return j;
		case JSMN_ARRAY:
		  for (i=0;i<t->size;i++) 
			  j+=json_get_token_size(t+j);
		  return j;
		default:
		  return 1;
	}
}

int json_get_token(json_object* json, char *key, json_object* result) {
 int i,n;
 jsmntok_t* c;

 for (i=1;i<json->tokc;i++) {
	 c = json->tok+i;
	 n = c->end - c->start;
	 // the key must be a string
	 if (c->type != JSMN_STRING)
	    return 0;
	 // if the key matches we retrun the next token 
	 if (strlen(key)==n && !strncmp(json->js + c->start, key, n)) {
		 result->js = json->js;
		 result->tok = c+1;
		 result->tokc = json->tokc -i;
		 return 1;
	 }
	 // if not we have to check the value
	 i+=1+json_get_token_size(c+1);
 }
 return 0;
}


char* json_array_get_one_str(char *buf, int *n, jsmntok_t **tok)
{
	int c;
	char *value;

	if (*n == 0)
		return NULL;

	c = (*tok)->end - (*tok)->start;
	value = calloc(1, (c+1) * sizeof(char));
	strncpy(value, buf+(*tok)->start, c);
	(*tok)++;
	(*n)--;

	return value;
}

int json_get_key_count(char *buf, char *key, jsmntok_t *tok, int tokc)
{
	int count = 0;

	for (int i=0; i<tokc; i++) {
		int n = tok[i].end - tok[i].start;
		if (tok[i].type == JSMN_STRING
		    && !strncmp(buf + tok[i].start, key, n)) {
			count += 1;
		}
	}

	return count;
}


int size_of_bytes(int str_len){
    int out_len = (str_len & 1) ? (str_len + 1) / 2 : str_len/2;
    return out_len;
}

uint8_t strtohex(char c)
{
    if(c >= '0' && c <= '9')
        return c - '0';
    if(c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if(c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 255;
}

int hex2byte_arr(char *buf, int len, uint8_t *out, int outbuf_size) {
    int i = len - 1;
    int out_len = (len & 1) ? (len + 1) / 2 : len/2;
    int j = out_len - 1;

    if (j > outbuf_size)
        return -1; /* Output buffer is smaller than need */

    while(i >= 0) {
        out[j] = strtohex(buf[i--]);
        if (i >= 0) {
            out[j--] |= strtohex(buf[i--]) << 4;
        }
    }

    return out_len;
}
bytes_t* hex2byte_bytes(char *buf, int len) {
    int bytes_len = (len & 1) ? (len + 1) / 2 : len/2;

    uint8_t *b  = malloc(bytes_len);
	hex2byte_arr(buf,len,b,bytes_len);
	return b_new(b,bytes_len);
}

void int8_to_char(uint8_t *buffer, int len, char *out) {
    const char hex[] = "0123456789abcdef";
    int max = 2 * len;
    int i = 0;
    int j = 0;
    while (j < len) {
        out[i++] = hex[(buffer[j] >> 4) & 0xF];
        out[i++] = hex[buffer[j] & 0xF];
        j++;
    }
    out[i] = '\0';
}



bytes_t *sha3(bytes_t *data)
{
	bytes_t *out;
	struct SHA3_CTX ctx;
	char p[65] = { '0' };

	out = calloc(1, sizeof(bytes_t));

	sha3_256_Init(&ctx);
	sha3_Update(&ctx, data->data, data->len);

	out->data = calloc(1, 32 * sizeof(uint8_t));
	out->len = 32;

	keccak_Final(&ctx, out->data);

	for(int i=0; i<out->len; i++) {
		char s[3];
		byte_to_hex(out->data[i],s);
		p[i*2] = s[0];
		p[(i*2)+1] = s[1];
	}
	dbg_log("sha3(): '%s'\n", p);

	return out;
}
