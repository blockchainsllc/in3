#include "stringbuilder.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

static const size_t MIN_SIZE = 32;

sb_t* sb_new(char* chars) {
    sb_t* sb = malloc(sizeof(sb_t));
    sb->data = malloc(MIN_SIZE);
    sb->allocted = MIN_SIZE;
    sb->data[0] = 0;
    sb->len =0;
    if (chars!=NULL)
      sb_add_chars(sb, chars);
    return sb;
}
static void check_size(sb_t* sb, size_t len) {
    if (sb==NULL || len==0 || sb->len + len < sb->allocted) return;
    while (sb->len + len >= sb->allocted)
       sb->allocted <<= 1;
    sb->data = realloc(sb->data,sb->allocted);
}

sb_t* sb_add_chars(sb_t* sb, char* chars) {
    int l = strlen(chars);
    if (l==0 || chars==NULL) return sb;
    check_size(sb,l);
    memcpy(sb->data + sb->len,chars,l);
    sb->data[sb->len]=0;
    return sb;
}


sb_t* sb_add_range(sb_t* sb, char* chars, int start, int len) {
    if (chars==NULL) return sb;
    check_size(sb,len);
    memcpy(sb->data + sb->len,chars+start,len);
    sb->data[sb->len]=0;
    return sb;
}


void sb_free(sb_t* sb) {
    if (sb==NULL) return;
    if (sb->data!=NULL) 
       free(sb->data);
    free(sb);
}