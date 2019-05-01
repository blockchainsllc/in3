#include "core/util/data.h"
#include <stdio.h>

#ifndef _Included_in3_IN3
#define _Included_in3_IN3
#ifdef __cplusplus
extern "C" {
#endif

void print_error(char* msg);
void print_success(char* msg);
#define ERROR(s) printf("Error: %s", s)

int test_evm(d_token_t* test, uint32_t props, uint64_t* ms);
int test_trie(d_token_t* test, uint32_t props, uint64_t* ms);
int test_rlp(d_token_t* test, uint32_t props, uint64_t* ms);

#ifdef __cplusplus
}
#endif
#endif
