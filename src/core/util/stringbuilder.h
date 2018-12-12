#ifndef __STR_BUILDER_H__
#define __STR_BUILDER_H__
 
#include <stddef.h>



typedef struct sb {
    char* data;
    size_t allocted;
    size_t len;
} sb_t;

sb_t* sb_new(char* chars);
void  sb_free(sb_t* sb);


sb_t* sb_add_chars(sb_t* sb, char* chars);
sb_t* sb_add_range(sb_t* sb, char* chars, int start, int len);

#endif


