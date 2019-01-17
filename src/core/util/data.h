/** @file 
 * json-parser.
 * 
 * The parser can read from :
 * - json
 * - bin
 * 
 * When reading from json all '0x'... values will be stored as bytes_t. If the value is lower than 0xFFFFFFF, it is converted as integer.
 * 
 * 
 * */

#include "bytes.h"
#include "stringbuilder.h"
#include <stdint.h>

#ifndef __DATA_H__
#define __DATA_H__

typedef uint16_t d_key_t;
/** type of a token. */
typedef enum {
  T_BYTES   = 0, /**< content is stored as data ptr. */
  T_STRING  = 1, /**<content is stored a c-str*/
  T_ARRAY   = 2, /**< the node is an array with the length stored in length */
  T_OBJECT  = 3, /**< the node is an object with properties*/
  T_BOOLEAN = 4, /**< boolean with the value stored in len */
  T_INTEGER = 5, /**< a integer with the value stored */
  T_NULL    = 6
} d_type_t;

/** a token holding any kind of value. 
 * 
 * use d_type,  d_len or the cast-function to get the value.
 */
typedef struct item {
  uint32_t len;  /**< the length of the content (or number of properties) depending +  type. */
  uint8_t* data; /**< the byte or string-data  */
  d_key_t  key;  /**< the key of the property. */
} d_token_t;

typedef struct str_range {
  char*  data;
  size_t len;
} str_range_t;

typedef struct json_parser {
  d_token_t* items;
  size_t     allocated;
  size_t     len;
  char*      c;
} json_parsed_t;

d_key_t key(char* c);
d_key_t keyn(char* c, int len);

bytes_t*  d_create_bytes(d_token_t* item);
bytes_t   d_to_bytes(d_token_t* item);
int       d_bytes_to(d_token_t* item, uint8_t* dst, int max);
bytes_t*  d_bytes(d_token_t* item);
char*     d_string(d_token_t* item);
int       d_int(d_token_t* item);
int       d_intd(d_token_t* item, int def_val);
uint64_t  d_long(d_token_t* item);
uint64_t  d_longd(d_token_t* item, uint64_t def_val);
bytes_t** d_create_bytes_vec(d_token_t* arr);

d_type_t d_type(d_token_t* item);
int      d_len(d_token_t* item);
size_t   d_token_size(d_token_t* item);
int      d_eq(d_token_t* a, d_token_t* b);

d_token_t* d_get(d_token_t* item, uint16_t key);
d_token_t* d_get_or(d_token_t* item, uint16_t key1, uint16_t key2);
d_token_t* d_get_at(d_token_t* item, int index);
d_token_t* d_next(d_token_t* item);

void           d_serialize_binary(bytes_builder_t* bb, d_token_t* t);
json_parsed_t* parse_binary(bytes_t* data);
json_parsed_t* parse_binary_str(char* data, int len);
json_parsed_t* parse_json(char* js);
void           free_json(json_parsed_t* parser_ctx);
str_range_t    d_to_json(d_token_t* item);
char*          d_create_json(d_token_t* item);

int   json_get_int_value(char* js, char* prop);
void  json_get_str_value(char* js, char* prop, char* dst);
char* json_get_json_value(char* js, char* prop);

char* d_get_keystr(d_key_t k);
void  d_track_keynames(uint8_t v);
void  d_clear_keynames();

#define d_get_string(r, k) d_string(d_get(r, key(k))) /**< reads string of a property as string. */
#define d_get_stringk(r, k) d_string(d_get(r, k))     /**< reads string of a property as key. */
#define d_get_string_at(r, i) d_string(d_get_at(r, i))

#define d_get_int(r, k) d_int(d_get(r, key(k)))
#define d_get_intk(r, k) d_int(d_get(r, k))
#define d_get_int(r, k) d_int(d_get(r, key(k)))
#define d_get_intkd(r, k, d) d_intd(d_get(r, k), d)
#define d_get_int_at(r, i) d_int(d_get_at(r, i))

#define d_get_long(r, k) d_long(d_get(r, key(k)))
#define d_get_longk(r, k) d_long(d_get(r, k))
#define d_get_long(r, k) d_long(d_get(r, key(k)))
#define d_get_longkd(r, k, d) d_longd(d_get(r, k), d)
#define d_get_long_at(r, i) d_long(d_get_at(r, i))

#define d_get_bytes(r, k) d_bytes(d_get(r, key(k)))
#define d_get_bytesk(r, k) d_bytes(d_get(r, k))
#define d_get_bytes_at(r, i) d_bytes(d_get_at(r, i))

#define d_is_binary_ctx(parsed) (parsed->allocated == 0)

#endif