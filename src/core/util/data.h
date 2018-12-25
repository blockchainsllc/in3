#include "bytes.h"
#include <stdint.h>
#include "stringbuilder.h"


#ifndef __DATA_H__
#define __DATA_H__

typedef uint16_t d_key_t;

typedef enum {
    T_BYTES = 0,
    T_STRING = 1,
    T_ARRAY = 2,
    T_OBJECT = 3,
    T_BOOLEAN = 4,
    T_INTEGER = 5,
    T_NULL = 6
} d_type_t;

typedef struct item {
    uint32_t len;        /**< the length of the array ion bytes */
    uint8_t *data;  /**< the byte-data  */
    uint16_t key;
} d_token_t;


typedef struct str_range {
    char* data;
    size_t len;
} str_range_t;

typedef struct json_parser {
    d_token_t* items;
    int allocated;
    int len;
    char* c;
} json_parsed_t;

d_key_t key(char* c);
d_key_t keyn(char* c, int len);
bytes_t* d_bytes(d_token_t* item);
char* d_string(d_token_t* item);
int d_int(d_token_t* item);
int d_intd(d_token_t* item, int def_val);
uint64_t d_long(d_token_t* item);
uint64_t d_longd(d_token_t* item, uint64_t def_val);
d_type_t d_type(d_token_t* item);
int d_len(d_token_t* item);
int d_token_size(d_token_t* item);
bytes_t ** d_create_bytes_vec(d_token_t* arr);


d_token_t*  d_get(d_token_t* item, uint16_t key);
d_token_t*  d_get_or(d_token_t* item, uint16_t key1, uint16_t key2);
d_token_t*  d_get_at(d_token_t* item, int index);
d_token_t*  d_next(d_token_t* item);

json_parsed_t* parse_json(char* js );
void free_json(json_parsed_t* parser_ctx);

str_range_t d_to_json(d_token_t* item);
char* d_create_json(d_token_t* item);


#define d_get_string(r,k) d_string(d_get(r,key(k)))
#define d_get_stringk(r,k) d_string(d_get(r,k))
#define d_get_string_at(r,i) d_string(d_get_at(r,i))

#define d_get_int(r,k) d_int(d_get(r,key(k)))
#define d_get_intk(r,k) d_int(d_get(r,k))
#define d_get_int(r,k) d_int(d_get(r,key(k)))
#define d_get_intkd(r,k,d) d_intd(d_get(r,k),d)
#define d_get_int_at(r,i) d_int(d_get_at(r,i))

#define d_get_long(r,k) d_long(d_get(r,key(k)))
#define d_get_longk(r,k) d_long(d_get(r,k))
#define d_get_long(r,k) d_long(d_get(r,key(k)))
#define d_get_longkd(r,k,d) d_longd(d_get(r,k),d)
#define d_get_long_at(r,i) d_long(d_get_at(r,i))

#define d_get_bytes(r,k) d_bytes(d_get(r,key(k)))
#define d_get_bytesk(r,k) d_bytes(d_get(r,k))
#define d_get_bytes_at(r,i) d_bytes(d_get_at(r,i))


#endif