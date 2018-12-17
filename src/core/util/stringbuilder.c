#include "stringbuilder.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../util/bytes.h"
#include "../util/utils.h"
#include "mem.h"

static const size_t MIN_SIZE = 32;

sb_t* sb_new(char* chars) {
    sb_t* sb = _malloc(sizeof(sb_t));
    sb->data = _malloc(MIN_SIZE);
    sb->allocted = MIN_SIZE;
    sb->data[0] = 0;
    sb->len =0;
    if (chars!=NULL)
      sb_add_chars(sb, chars);
    return sb;
}
sb_t* sb_init(sb_t* sb) {
    sb->data = _malloc(MIN_SIZE);
    sb->allocted = MIN_SIZE;
    sb->data[0] = 0;
    sb->len =0;
    return sb;
}
static void check_size(sb_t* sb, size_t len) {
    if (sb==NULL || len==0 || sb->len + len < sb->allocted) return;
    size_t l= sb->allocted;
    while (sb->len + len >= sb->allocted)
       sb->allocted <<= 1;
    sb->data = k_realloc(sb->data,sb->allocted,l);
}

sb_t* sb_add_chars(sb_t* sb,  char* chars) {
    int l = strlen(chars);
    if (l==0 || chars==NULL) return sb;
    check_size(sb,l);
    memcpy(sb->data + sb->len,chars,l);
    sb->len+=l;
    sb->data[sb->len]=0;
    return sb;
}
sb_t* sb_add_char(sb_t* sb, char c) {
    check_size(sb,1);
    sb->data[sb->len++]=c;
    sb->data[sb->len]=0;
    return sb;
}


sb_t* sb_add_range(sb_t* sb, char* chars, int start, int len) {
    if (chars==NULL) return sb;
    check_size(sb,len);
    memcpy(sb->data + sb->len,chars+start,len);
    sb->len+=len;
    sb->data[sb->len]=0;
    return sb;
}
sb_t* sb_add_key_value(sb_t* sb, char* key, char* value, int lv, bool as_string) {
    if (lv==0) return sb;
    int p=sb->len, lk=strlen(key);
    check_size(sb,(as_string ? 2 : 0)+lk+3+lv);
    sb->data[p++]='"';
    memcpy(sb->data+p,key,lk);
    p+=lk;
    sb->data[p++]='"';
    sb->data[p++]=':';
    if (as_string)  sb->data[p++]='"';
    memcpy(sb->data+p,value,lv);
    p+=lv;
    if (as_string)  sb->data[p++]='"';
    sb->len=p;
    sb->data[sb->len]=0;
    return sb;
}


sb_t* sb_add_bytes(sb_t* sb, char* prefix, bytes_t* bytes, int len, bool as_array) {
    int p=sb->len, lk=prefix==NULL?0:strlen(prefix), s=0,i;
    for (i=0;i<len;i++) s+=bytes[i].len*2+4+(i>0?1:0);
    check_size(sb, s+lk+(as_array ? 2:0 ));
    if (prefix!=NULL)
      memcpy(sb->data+p,prefix,lk);
    p+=lk;

    if (as_array) sb->data[p++]='[';
    for (i=0;i<len;i++)  {
        if (i>0) sb->data[p++]=',';
        sb->data[p++]='"';
        sb->data[p++]='0';
        sb->data[p++]='x';
        int8_to_char(bytes[i].data,bytes[i].len,sb->data+p);
        p+=bytes[i].len*2;
        sb->data[p++]='"';
    }
    if (as_array) sb->data[p++]=']';
    sb->data[p]=0;
    sb->len=p;
    return sb;
}


void sb_free(sb_t* sb) {
    if (sb==NULL) return;
    if (sb->data!=NULL) 
      _free(sb->data);
   _free(sb);
}