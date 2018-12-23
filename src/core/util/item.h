#include "bytes.h"
#include <stdint.h>
#include "stringbuilder.h"

typedef uint16_t r_key_t;

typedef enum {
    T_BYTES = 0,
    T_STRING = 1,
    T_ARRAY = 2,
    T_OBJECT = 3,
    T_BOOLEAN = 4,
    T_INTEGER = 5,
    T_NULL = 6
} r_type_t;

typedef struct item {
    uint32_t len;        /**< the length of the array ion bytes */
    uint8_t *data;  /**< the byte-data  */
    uint16_t key;
} r_item_t;


typedef struct str_range {
    char* data;
    size_t len;
} str_range_t;



// len>0 = bytes or string
// len = 0  -> NULL
// len = -1 -> boolean true 
// len = -2 -> boolean false
// len = -3  -> byte

r_key_t key(char* c);
r_key_t keyn(char* c, int len);
bytes_t* r_bytes(r_item_t* item);
char* r_string(r_item_t* item);
int r_int(r_item_t* item);
r_type_t r_type(r_item_t* item);
int child_count(r_item_t* item);
int r_size(r_item_t* item);


r_item_t*  r_get(r_item_t* item, uint16_t key);
r_item_t*  r_get_at(r_item_t* item, int index);

int parse_json(char* js, r_item_t** items, int* tokc );
void free_json(r_item_t* items, int tokc);

str_range_t r_to_json(r_item_t* item);

#define r_get_string(r,k) r_string(r_get(r,key(k)))
#define r_get_stringk(r,k) r_string(r_get(r,k))
#define r_get_string_at(r,i) r_string(r_get_at(r,i))

#define r_get_int(r,k) r_int(r_get(r,key(k)))
#define r_get_intk(r,k) r_int(r_get(r,k))
#define r_get_int_at(r,i) r_int(r_get_at(r,i))

#define r_get_bytes(r,k) r_bytes(r_get(r,key(k)))
#define r_get_bytesk(r,k) r_bytes(r_get(r,k))
#define r_get_bytes_at(r,i) r_bytes(r_get_at(r,i))
