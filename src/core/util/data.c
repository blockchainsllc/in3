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
        val^=*c | val<<9;
        c+=1;
    }
    return val;
}
d_key_t keyn(char* c, int len) {
    uint16_t val=0;
    int i=0;
    for (;i<len;i++) {
        if (*c==0) return val;
        val^=*c | val<<9;
        c+=1;
    }
    return val;
}

bytes_t* d_bytes(d_item_t* item) {
    return (bytes_t*) item;
}
char* d_string(d_item_t* item) {
    if (item==NULL) return NULL;
    return (char*)item->data;
}
int d_int(d_item_t* item) {
    if (item==NULL) return 0;
    if (d_type(item)==T_INTEGER)
      return item->len & 0xFFFFFFF;
    else if (d_type(item) == T_BYTES) {
        return 0;
    }
    return 0;

}




d_type_t d_type(d_item_t* item) {
    return item==NULL ? T_NULL :(item->len & 0xF0000000)>>28;
}
int child_count(d_item_t* item) {
    if (item==NULL) return 0;
    switch (d_type(item)) {
        case T_ARRAY:
        case T_OBJECT:
          return item->len & 0xFFFFFFF;
        default:
          return 0;
    }
}

int d_size(d_item_t* item) {
    if (item==NULL) return 0;
    int i,c=1;
    switch (d_type(item)) {
        case T_ARRAY:
        case T_OBJECT:
          for (i=0;i<(item->len & 0xFFFFFFF);i++)
            c+=d_size(item+c);
          return c;
        default:
          return 1;
    }
}

d_item_t*  d_get(d_item_t* item, uint16_t key) {
  if (item==NULL) return NULL;
  int i=0,l=item->len & 0xFFFFFFF;
  item+=1;
  for (;i<l;i++,item+=d_size(item)) {
      if (item->key==key) return item;
  }
  return NULL;
}

d_item_t*  d_get_at(d_item_t* item, int index) {
  if (item==NULL) return NULL;
  int i=0,l=item->len & 0xFFFFFFF;
  item+=1;
  for (;i<l;i++,item+=d_size(item)) {
      if (i==index) return item;
  }
  return NULL;
}

d_item_t*  d_next(d_item_t* item) {
   return item==NULL ? NULL : item+d_size(item);
}

typedef struct json_parser {
    d_item_t* items;
    int allocated;
    int len;
    char* c;
} json_parsed_t;



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

d_item_t* parsed_next_item(json_parsed_t* jp, d_type_t type, d_key_t key,int parent) {
  if (jp->len+1>jp->allocated) {
      jp->items = _realloc(jp->items,(jp->allocated<<1)*sizeof(d_item_t),jp->allocated*sizeof(d_item_t));
      jp->allocated<<=1;
  }
  d_item_t* n = jp->items+jp->len;
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

int parse_number(json_parsed_t* jp, d_item_t* item) {
    char temp[20];
    int i=0;
    jp->c--;
    char* start=jp->c;
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

int parse_string(json_parsed_t* jp, d_item_t* item) {
    char* start=jp->c;
    int l,i;
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
          parsed_next_item(jp,T_ARRAY,key,parent);
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

void free_json(d_item_t* items, int tokc) {
    if (items==NULL) return;
    int i;
    for (i=0;i<tokc;i++) {
        if (items[i].data!=NULL && d_type(items+i)<2) 
            _free(items[i].data);
    }
    _free(items);
}

int parse_json(char* js, d_item_t** items, int* tokc ) {
    json_parsed_t parser={ .items = _malloc(sizeof(d_item_t)*10), .allocated=10, .len=0, .c=js };
    int res = parse_object(&parser,-1,0);
    if (res<0) free_json(parser.items,parser.len);
    else {
        *items = parser.items;
        *tokc  = parser.len;
    }
    return res;

}



static int find_end(char* str) {
    int l=0;
    char* c=str;
    while (*c!=0) {
        switch (*(c++)) {
            case '{': l++; break;
            case '}': l--; break;
        }
        if (l==0) 
           return c - str;
    }
    return c - str;
}


str_range_t d_to_json(d_item_t* item) {
    str_range_t s;
    s.data =  (char*) item->data;
    s.len = find_end(s.data);
    return s;
}
