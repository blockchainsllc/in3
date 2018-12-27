#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "bytes.h"
#include "mem.h"
#include "debug.h"
#include "../crypto/sha3.h"
#include "utils.h"

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

	*buf = _calloc(1, (len + 3) * sizeof(char));
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

	*buf = _malloc(nbytes);
	if (*buf == 0)
		return 0;

	if (hex2byte_arr(str+off, size, *buf, nbytes))
		return nbytes;

	_free(*buf);
	return 0;
}


int size_of_bytes(int str_len){
    int out_len = (str_len & 1) ? (str_len + 1) / 2 : str_len/2;
    return out_len;
}
void long_to_bytes(uint64_t val, uint8_t* dst) {
	*dst     = val >> 56 & 0xFF;
	*(dst+1) = val >> 48 & 0xFF;
	*(dst+2) = val >> 40 & 0xFF;
	*(dst+3) = val >> 32 & 0xFF;
	*(dst+4) = val >> 24 & 0xFF;
	*(dst+5) = val >> 16 & 0xFF;
	*(dst+6) = val >>  8 & 0xFF;
	*(dst+7) = val       & 0xFF;
}
void int_to_bytes(uint32_t val, uint8_t* dst) {
	*dst     = val >> 24 & 0xFF;
	*(dst+1) = val >> 16 & 0xFF;
	*(dst+2) = val >>  8 & 0xFF;
	*(dst+3) = val       & 0xFF;
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
bytes_t* hex2byte_new_bytes(char *buf, int len) {
    int bytes_len = (len & 1) ? (len + 1) / 2 : len/2;

    uint8_t *b  = _malloc(bytes_len);
	hex2byte_arr(buf,len,b,bytes_len);
	bytes_t* bytes = _malloc(sizeof(bytes_t));
	bytes->data = b;
	bytes->len=bytes_len;
	return bytes;
}

void int8_to_char(uint8_t *buffer, int len, char *out) {
    const char hex[] = "0123456789abcdef";
    int i = 0,j = 0;
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

	out = _calloc(1, sizeof(bytes_t));

	sha3_256_Init(&ctx);
	sha3_Update(&ctx, data->data, data->len);

	out->data = _calloc(1, 32 * sizeof(uint8_t));
	out->len = 32;

	keccak_Final(&ctx, out->data);

    #ifdef DEBUG
	char p[65] = { '0' };
	for(size_t i=0; i<out->len; i++) {
		char s[3];
		byte_to_hex(out->data[i],s);
		p[i*2] = s[0];
		p[(i*2)+1] = s[1];
	}
	dbg_log("sha3(): '%s'\n", p);
	#endif

	return out;
}

uint64_t bytes_to_long (uint8_t* data, int len) {
	uint64_t res = 0;
	int i;
	for (i=0;i<len;i++) {
		if (data[i])
   		  res |= ((uint64_t) data[i] ) << (len-i-1)*8; 
	}
	return res;
}
long c_to_long(char* a, int l)  {
	if (a[0]=='0' && a[1]=='x') {
   	    long val=0;
        for (int i=l-1;i>1;i--) 
            val |= ((uint64_t) strtohex(a[i])) << ( 4*(l-1-i) );
		return val;
    }
	else if (l<12) {
		char temp[12];
		strncpy(temp,a,l);
		temp[l]=0;
		return atoi(temp);
	}
	return -1;

}

char c_to_lower(char c) {
	return c>='A' && c<='Z' ? c-32 : c;
}


bool equals_range(char* a, int la,char* b, int lb, uint8_t mode) {
    if (a==NULL) return b==NULL;
    if (b==NULL) return a==NULL;
    if (EQ_MODE_CASE_NUMBER & mode) 
	   return c_to_long(a,la)==c_to_long(b,lb);
	if (la!=lb) return false;
	if (EQ_MODE_CASE_INSENSITIVE & mode) {
   	  int i;
	  for (i=0;i<la;i++) {
		  if (a[i]!=b[i] && c_to_lower(a[i])==c_to_lower(b[i])) return false;
	  }
	  return true;
	}
	return strncmp(a,b,la)==0;
}
char* _strdup(char* src, int len) {
	if (len<0) len=strlen(src);
	char* dst = _malloc(len+1);
	strncpy(dst,src,len);
	dst[len]=0;
	return dst;
}
int min_bytes_len(uint64_t val) {
	int i;
	for (i=0;i<8;i++,val>>=8) {
		if (val==0) return i;
	}
	return 8;
}
