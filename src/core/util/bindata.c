#include "data.h"
#include "stringbuilder.h"
#include "utils.h"
#include "bytes.h"


static d_token_t* next_item(json_parsed_t* jp, d_type_t type,  int len) {
  if (jp->allocated==0) {
      jp->items = _malloc(10*sizeof(d_token_t));
      jp->allocated=10;
  }
  else if (jp->len+1>jp->allocated) {
      jp->items = _realloc(jp->items,(jp->allocated<<1)*sizeof(d_token_t),jp->allocated*sizeof(d_token_t));
      jp->allocated<<=1;
  }
  d_token_t* n = jp->items+jp->len;
  jp->len+=1;
  n->key=0;
  n->data=NULL;
  n->len=type<<28 | len;
  return n;
}

static int read_token(json_parsed_t* jp, uint8_t* d, size_t* p) {
    d_type_t type = d[*p] >> 5;
    uint32_t len  = d[(*p)++] & 0x1F,i;
    uint16_t key;

    if (len==28)      len = d[(*p)++]; 
    else if (len==29) len = d[(*p)++]<<8 | d[(*p)++]; 
    else if (len==30) len = d[(*p)++]<<16 | d[(*p)++]<<8 | d[(*p)++]; 
    else if (len==31) len = d[(*p)++]<<24 | d[(*p)++]<<16 | d[(*p)++]<<8 | d[(*p)++]; 

    // special token giving the number of tokens, so we can allocate the exact number
    if (type==T_NULL && len>0) {
        if (jp->allocated=0) {
            jp->items=_malloc(sizeof(d_item_t)*len);
            jp->allocated = len;
        }
        else if (len>jp->allocated) {
            jp->items = _realloc(jp->items,len*sizeof(d_token_t),jp->allocated*sizeof(d_token_t));
            jp->allocated=len;
        }
        return 0;
    }
    d_item_t* t = next_item(jp,type,len);
    switch(type) {
        case T_ARRAY:
          for (i=0;i<len;i++) {
              if (read_token(jp,d,p)) return 1;
              jp->items[jp->len-1].key=i;
          }
          break;
        case T_OBJECT:
          for (i=0;i<len;i++) {
              key = d[(*p)++]<<8 | d[(*p)++]; 
              if (read_token(jp,d,p)) return 1;
              jp->items[jp->len-1].key=key;
          }
          break;
        case T_STRING:
          t->data = d+((*p)++);
          if (t->data[len]!=0) return 1;
          *p+=len;
          break;
        case T_BYTES:
          t->data = d+(*p));
          *p+=len;
          break;
        default:
          break;
    }
    return 0;
}

json_parsed_t* d_read_tokens(bytes_t* data) {
    size_t p=0, error=0;
    json_parsed_t* jp= _calloc(1,sizeof(json_parsed_t));
    jp->c=data->data;

    while (!error && p<data->len) 
        error = read_token(jp, data->data, &p);

    if (error) {
        _free(jp->items);
        _free(jp);
        return NULL;
    }
    jp->allocated=0;
    return jp;
}

void d_write_token_count(bytes_builder_t* bb, int len) {
    bb_write_byte(bb, T_NULL <<5 | (len<28 ? len: min_bytes_len(len)+27));
    if (len>27) 
      bb_write_long_be(bb,len,min_bytes_len(len));
}


void d_write_token(bytes_builder_t* bb, d_token_t* t) {
    int len  = d_len(t),i;
    d_token_t* c;
    bb_write_byte(bb, d_type(t)<<5 | (len<28 ? len: min_bytes_len(len)+27));
    if (len>27) 
      bb_write_long_be(bb,len,min_bytes_len(len));

     switch (d_type(t)) {
        case T_ARRAY:
          for (i=0,c=t+1;i<len;i++,c=d_next(c)) d_write_token(bb,c);
          break;
        case T_BYTES:
          bb_write_raw_bytes(bb,t->data,len);
          break;
        case T_OBJECT:
          for (i=0,c=t+1;i<len;i++,c=d_next(c)) {
              bb_write_long_be(bb,2,t->key);
              d_write_token(bb,c);
          }
          break;
        case T_STRING:
          bb_write_raw_bytes(bb,t->data,len+1);
          break;
        case T_BOOLEAN:
        case T_INTEGER:
        case T_NULL:
          break;
    }
}