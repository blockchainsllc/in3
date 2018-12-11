

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "bytes.h"
#include "../jsmn/jsmn.h"

typedef uint32_t pb_size_t;
typedef uint_least8_t pb_byte_t;

#define CLONE(str) char* =     bytes_t v; uint8_t d[20];  v.data=d; v.len=20;


#define JSON_OBJECT(ob, str, tokens) json_object_t ob = { .js=str, .tok=tokens };


int size_of_bytes(int str_len);
uint8_t strtohex(char c);
int hex2byte_arr(char *buf, int len, uint8_t *out, int outbuf_size);
bytes_t* hex2byte_new_bytes(char *buf, int len);
void int8_to_char(uint8_t *buffer, int len, char *out);

bytes_t *sha3(bytes_t *data);

void byte_to_hex(uint8_t b, char s[23]);
int hash_cmp(uint8_t *a, uint8_t *b);


int json_get_int_value(char *data, char *key);
char *json_get_value(char *data, char *key);


int str2hex_str(char *str, char **buf);
int str2byte_a(char *str, uint8_t **buf);
void *k_realloc(void *ptr, size_t size, size_t oldsize);
int get_json_key_value_int(char *buf, char *key, jsmntok_t* tok, int tokc);
char* get_json_key_value(char *buf, char *key, jsmntok_t* tok, int tokc);
char* json_array_get_one_str(char *buf, int *n, jsmntok_t **tok);




#endif
