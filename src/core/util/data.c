#include "bytes.h"
#include "utils.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "data.h"
#include "mem.h"
#include "stringbuilder.h"

// len>0 = bytes or string
// len = 0  -> NULL
// len = -1 -> boolean true 
// len = -2 -> boolean false
// len = -3  -> byte

d_key_t key(char* c) {
    uint16_t val=0;
    while (true) {
        if (*c==0) return val;
        val^=*c | val<<7;
        c+=1;
    }
    return val;
}
d_key_t keyn(char* c, int len) {
    uint16_t val=0;
    int i=0;
    for (;i<len;i++) {
        if (*c==0) return val;
        val^=*c | val<<7;
        c+=1;
    }
    return val;
}

bytes_t* d_bytes(d_token_t* item) {
    return (bytes_t*) item;
}
char* d_string(d_token_t* item) {
    if (item==NULL) return NULL;
    return (char*)item->data;
}
int d_int(d_token_t* item) {
    return d_intd(item,0);
}
int d_intd(d_token_t* item, int def_val) {
    if (item==NULL) return def_val;
    switch (d_type(item)) {
        case T_INTEGER:
        case T_BOOLEAN:
          return item->len & 0xFFFFFFF;
        default:
          return def_val;
    }
}

bytes_t ** d_create_bytes_vec(d_token_t* arr) {
    if (arr==NULL) return NULL;
    int l = d_len(arr),i;
    bytes_t ** dst = _calloc(l+1,sizeof(bytes_t*)) ;
    d_token_t* t = arr+1;
    for (i=0;i<l;i++, t+=d_token_size(t)) 
       dst[i]=d_bytes(t); 
    return dst;
}

uint64_t d_long(d_token_t* item) {
    return d_longd(item,0L);
}
uint64_t d_longd(d_token_t* item, uint64_t def_val) {
    if (item==NULL) return def_val;
    if (d_type(item)==T_INTEGER)
      return item->len & 0xFFFFFFF;
    else if (d_type(item) == T_BYTES) {
        return 0;
    }
    return 0;    
}



d_type_t d_type(d_token_t* item) {
    return item==NULL ? T_NULL :(item->len & 0xF0000000)>>28;
}
int d_len(d_token_t* item) {
    return item==NULL ? 0 : item->len & 0xFFFFFFF;
}

size_t d_token_size(d_token_t* item) {
    if (item==NULL) return 0;
    size_t i,c=1;
    switch (d_type(item)) {
        case T_ARRAY:
        case T_OBJECT:
          for (i=0;i<(item->len & 0xFFFFFFF);i++)
            c+=d_token_size(item+c);
          return c;
        default:
          return 1;
    }
}

int d_eq(d_token_t* a, d_token_t* b) {
    if (a==NULL || b==NULL) return false;
    if (a->len!=b->len) return false;
    if (a->data && b->data) 
        return b_cmp(d_bytes(a),d_bytes(b));
    return a->data==NULL && b->data==NULL;
}

d_token_t*  d_get(d_token_t* item, uint16_t key) {
  if (item==NULL) return NULL;
  int i=0,l=item->len & 0xFFFFFFF;
  item+=1;
  for (;i<l;i++,item+=d_token_size(item)) {
      if (item->key==key) return item;
  }
  return NULL;
}
d_token_t*  d_get_or(d_token_t* item, uint16_t key, uint16_t key2) {
  if (item==NULL) return NULL;
  d_token_t* s=NULL;
  int i=0,l=item->len & 0xFFFFFFF;
  item+=1;
  for (;i<l;i++,item+=d_token_size(item)) {
      if (item->key==key) return item;
      if (item->key==key2) s=item;
  }
  return s;
}

d_token_t*  d_get_at(d_token_t* item, int index) {
  if (item==NULL) return NULL;
  int i=0,l=item->len & 0xFFFFFFF;
  item+=1;
  for (;i<l;i++,item+=d_token_size(item)) {
      if (i==index) return item;
  }
  return NULL;
}

d_token_t*  d_next(d_token_t* item) {
   return item==NULL ? NULL : item+d_token_size(item);
}


char next_char(json_parsed_t* jp) {
    while (true) {
        switch (*jp->c) {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
              jp->c++;
              break;
            default:
              return *(jp->c++);
        }
    }
}

d_token_t* parsed_next_item(json_parsed_t* jp, d_type_t type, d_key_t key,int parent) {
  if (jp->len+1>jp->allocated) {
      jp->items = _realloc(jp->items,(jp->allocated<<1)*sizeof(d_token_t),jp->allocated*sizeof(d_token_t));
      jp->allocated<<=1;
  }
  d_token_t* n = jp->items+jp->len;
  jp->len+=1;
  n->key=key;
  n->data=NULL;
  n->len=type<<28;
  if (parent>=0) jp->items[parent].len++;
  return n;
}

int parse_key(json_parsed_t* jp) {
    char* start=jp->c;
    int r;
    while (true) {
        switch (*(jp->c++)) {
            case 0: return -2;
            case '"':
              r = keyn(start,jp->c-start-1);
              return next_char(jp)==':' ? r : -2;
            case '\\' :
               jp->c++; 
               break;
        }
    }
}

int parse_number(json_parsed_t* jp, d_token_t* item) {
    char temp[20];
    int i=0;
    jp->c--;
    for (;i<20;i++) {
        if (jp->c[i]>='0' && jp->c[i]<='9') 
           temp[i]=jp->c[i];
        else {
            temp[i]=0;
            jp->c+=i;
            item->len |= atoi(temp);
            return 0;
        }
    }
    return -2;
}

int parse_string(json_parsed_t* jp, d_token_t* item) {
    char* start=jp->c;
    size_t l,i;
    while (true) {
        switch (*(jp->c++)) {
            case 0: return -2;
            case '"': 
               l = jp->c-start-1;
               if (l>1 && *start=='0' && start[1]=='x') {
                   // this is a hex-value
                   if (l<10) { // we can accept up to 3,4 bytes as integer 
                     item->len = T_INTEGER << 28;
                     for (i=2;i<l;i++) 
                       item->len |= strtohex(start[i]) << (l-i-1)*4;
                   }
                   else {
                       // we need to allocate bytes for it. and so set the type to bytes
                       item->len =(l%2?l-1:l-2)/2;
                       item->data=_malloc(item->len);
                       if (l%2)  item->data[0]=strtohex(start[2]);
                       l=l%2+2;
                       for (i=l-2;i<item->len;i++) 
                          item->data[i] = strtohex(start[l+i*2])<<4 | strtohex(start[l+i*2+1]) ;
                   }
               }
               else {
                   item->len = l | T_STRING <<28;
                   item->data = (uint8_t*) start;
                   item->data = _malloc(l+1);
                   memcpy(item->data,start,l);
                   item->data[l]=0;
               }
               return 0; 
            case '\\' : jp->c++; break;
        }
    }
}


int parse_object(json_parsed_t* jp, int parent, uint32_t key) {
    int res, p_index=jp->len;
    

    switch (next_char(jp)) {
        case 0: return -2;
        case '{':
          parsed_next_item(jp,T_OBJECT,key,parent)->data = (uint8_t*) jp->c-1;
          while (true) {
            switch (next_char(jp)) {
                case '"': 
                   res = parse_key(jp);
                   if (res<0) return res;
                   break;
                case '}':  return 0;
                default:   return -2;  // invalid character or end
            }
            res = parse_object(jp,p_index,res);  // parse the value
            if (res<0) return res;
            switch (next_char(jp)) {
                case ',': break;     // we continue reading the next property
                case '}': return 0;  // this was the last property, so we return successfully.
                default : return -2; // unexpected character, throw.
            }
          }
        case '[':
          parsed_next_item(jp,T_ARRAY,key,parent)->data = (uint8_t*) jp->c-1;
          if (next_char(jp)==']') return 0;
          jp->c--;

          while (true) {
            res = parse_object(jp,p_index, jp->items[p_index].len & 0xFFFFFF);  // parse the value
            if (res<0) return res;
            switch (next_char(jp)) {
                case ',': break;     // we continue reading the next property
                case ']': return 0;  // this was the last element, so we return successfully.
                default : return -2; // unexpected character, throw.
            }
          }
        case '"':
          return parse_string(jp,parsed_next_item(jp,T_STRING,key,parent));
        case 't':
          if (strncmp(jp->c,"rue",3)==0) {
              parsed_next_item(jp,T_BOOLEAN,key,parent)->len|=1;
              jp->c+=3;
              return 0;
          }
          else return -2;
        case 'f':
          if (strncmp(jp->c,"alse",4)==0) {
              parsed_next_item(jp,T_BOOLEAN,key,parent);
              jp->c+=4;
              return 0;
          }
          else return -2;
        case 'n':
          if (strncmp(jp->c,"ull",3)==0) {
              parsed_next_item(jp,T_NULL,key,parent);
              jp->c+=3;
              return 0;
          }
          else return -2;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
           return parse_number(jp,parsed_next_item(jp,T_INTEGER,key,parent) );
        default:
           return -2;
    }
 

}

void free_json(json_parsed_t* jp) {
    if (jp->items==NULL) return;
    if (jp->allocated) {
        size_t i;
        for (i=0;i<jp->len;i++) {
            if (jp->items[i].data!=NULL && d_type(jp->items+i)<2) 
                _free(jp->items[i].data);
        }
    }
    _free(jp->items);
    _free(jp);
}

json_parsed_t* parse_json(char* js ) {
    json_parsed_t* parser=_malloc(sizeof(json_parsed_t));
    parser->len = 0;
    parser->items = _malloc(sizeof(d_token_t)*10);
    parser->c = js;
    parser->allocated = 10;
    int res = parse_object(parser,-1,0);
    if (res<0) {
      free_json(parser);
      return NULL;
    }
    parser->c=js;
    return parser;
}



static int find_end(char* str) {
    int l=0;
    char* c=str;
    while (*c!=0) {
        switch (*(c++)) {
            case '{': 
            case '[': 
               l++; break;
            case '}': 
            case ']': 
               l--; break;
        }
        if (l==0) 
           return c - str;
    }
    return c - str;
}

char* d_create_json(d_token_t* item) {
    if (item==NULL) return NULL;
    char* dst;
    int l=d_len(item);
    str_range_t s;
    switch (d_type(item)) {
        case T_ARRAY:
        case T_OBJECT:
          s= d_to_json(item);
          dst = _malloc(s.len+1);
          memcpy(dst,s.data,s.len);
          dst[s.len]=0;
          return dst;
        case T_BOOLEAN:
          return d_int(item) ? _strdup("true",4) : _strdup("false",5);
        case T_INTEGER:
          dst = _malloc(16);
          sprintf(dst,"0x%x",d_int(item));
          return dst;
        case T_NULL:
          return _strdup("null",4);
        case T_STRING:
          dst = _malloc(l+3);
          dst[0]='"';
          dst[l+1]='"';
          dst[l+2]=0;
          memcpy(dst+1,item->data,l);
          return dst;
        case T_BYTES:
          dst = _malloc(l*2+5);
          dst[0]='"';
          dst[1]='0';
          dst[2]='x';
          int8_to_char(item->data,item->len,dst+3);
          dst[l*2+3]='"';
          dst[l*2+4]=0;
          return dst;
    }
    return NULL;
}

str_range_t d_to_json(d_token_t* item) {
    str_range_t s;
    s.data =  (char*) item->data;
    s.len = find_end(s.data);
    return s;
}


// util fast parse
int json_get_int_value(char* js, char* prop) {
    json_parsed_t* ctx = parse_json(js);
    if (ctx) {
      int res = d_get_int( ctx->items, prop);
      free_json(ctx);
      return res;
    }
    return -1;
}

char* json_get_str_value(char* js, char* prop) {
    json_parsed_t* ctx = parse_json(js);
    if (ctx) {
      char* c = d_get_string( ctx->items, prop);
      free_json(ctx);
      return c ? _strdup(c,strlen(c)) : NULL;
    }
    return NULL;
}

char* json_get_json_value(char* js, char* prop) {
    json_parsed_t* ctx = parse_json(js);
    if (ctx) {
      char* c = d_create_json( d_get( ctx->items, key(prop) ));
      free_json(ctx);
      return c;
    }
    return NULL;
}



//    bytes-parser



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
    uint16_t key;
    d_type_t type = d[*p] >> 5;

    // calculate len
    uint32_t len  = d[(*p)++] & 0x1F,i;
    int l = len>27 ? len-27 : 0;
    if (len==28)      len = d[*p]; 
    else if (len==29) len = d[*p]<<8 | d[*p+1]; 
    else if (len==30) len = d[*p]<<16 | d[*p+1]<<8 | d[*p+2]; 
    else if (len==31) len = d[*p]<<24 | d[*p+1]<<16 | d[*p+2]<<8 | d[*p+3]; 
    *p+=l;

    // special token giving the number of tokens, so we can allocate the exact number
    if (type==T_NULL && len>0) {
        if (jp->allocated==0) {
            jp->items=_malloc(sizeof(d_token_t)*len);
            jp->allocated = len;
        }
        else if (len>jp->allocated) {
            jp->items = _realloc(jp->items,len*sizeof(d_token_t),jp->allocated*sizeof(d_token_t));
            jp->allocated=len;
        }
        return 0;
    }
    d_token_t* t = next_item(jp,type,len);
    switch(type) {
        case T_ARRAY:
          for (i=0;i<len;i++) {
              if (read_token(jp,d,p)) return 1;
              jp->items[jp->len-1].key=i;
          }
          break;
        case T_OBJECT:
          for (i=0;i<len;i++) {
              key = d[(*p)]<<8 | d[*p+1]; 
              *p+=2;
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
          t->data = d+(*p);
          *p+=len;
          break;
        default:
          break;
    }
    return 0;
}

json_parsed_t* parse_binary(bytes_t* data) {
    size_t p=0, error=0;
    json_parsed_t* jp= _calloc(1,sizeof(json_parsed_t));
    jp->c=(char*) data->data;

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

static void write_token_count(bytes_builder_t* bb, int len) {
    bb_write_byte(bb, T_NULL <<5 | (len<28 ? len: min_bytes_len(len)+27));
    if (len>27) 
      bb_write_long_be(bb,len,min_bytes_len(len));
}

static void write_token(bytes_builder_t* bb, d_token_t* t) {
    int len  = d_len(t),i;
    d_token_t* c;
    bb_write_byte(bb, d_type(t)<<5 | (len<28 ? len: min_bytes_len(len)+27));
    if (len>27) 
      bb_write_long_be(bb,len,min_bytes_len(len));

     switch (d_type(t)) {
        case T_ARRAY:
          for (i=0,c=t+1;i<len;i++,c=d_next(c)) write_token(bb,c);
          break;
        case T_BYTES:
          bb_write_raw_bytes(bb,t->data,len);
          break;
        case T_OBJECT:
          for (i=0,c=t+1;i<len;i++,c=d_next(c)) {
              bb_write_long_be(bb,t->key,2);
              write_token(bb,c);
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

void d_serialize_binary(bytes_builder_t* bb, d_token_t* t) {
    write_token_count(bb,d_token_size(t));
    write_token(bb,t);
}

