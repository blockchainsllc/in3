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
} d_item_t;


typedef struct str_range {
    char* data;
    size_t len;
} str_range_t;



// len>0 = bytes or string
// len = 0  -> NULL
// len = -1 -> boolean true 
// len = -2 -> boolean false
// len = -3  -> byte

d_key_t key(char* c);
d_key_t keyn(char* c, int len);
bytes_t* d_bytes(d_item_t* item);
char* d_string(d_item_t* item);
int d_int(d_item_t* item);
d_type_t d_type(d_item_t* item);
int child_count(d_item_t* item);
int d_size(d_item_t* item);


d_item_t*  d_get(d_item_t* item, uint16_t key);
d_item_t*  d_get_at(d_item_t* item, int index);

int parse_json(char* js, d_item_t** items, int* tokc );
void free_json(d_item_t* items, int tokc);

str_range_t d_to_json(d_item_t* item);

#define d_get_string(r,k) d_string(d_get(r,key(k)))
#define d_get_stringk(r,k) d_string(d_get(r,k))
#define d_get_string_at(r,i) d_string(d_get_at(r,i))

#define d_get_int(r,k) d_int(d_get(r,key(k)))
#define d_get_intk(r,k) d_int(d_get(r,k))
#define d_get_int_at(r,i) d_int(d_get_at(r,i))

#define d_get_bytes(r,k) d_bytes(d_get(r,key(k)))
#define d_get_bytesk(r,k) d_bytes(d_get(r,k))
#define d_get_bytes_at(r,i) d_bytes(d_get_at(r,i))


#endif