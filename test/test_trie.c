

#ifndef TEST
#define TEST
#endif
#include <core/util/data.h>
#include <core/util/utils.h>
#include <eth_basic/trie.h>
#include <eth_full/evm.h>
#include <eth_nano/rlp.h>
#include <eth_nano/serialize.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "vm_runner.h"

bytes_t get_bytes(d_token_t* t, uint8_t* tmp, uint8_t is_hex) {
  bytes_t res;
  res = d_to_bytes(t);
  /*
  if (d_type(t) == T_BYTES && !is_hex) {
    tmp[0] = '0';
    tmp[1] = 'x';
    int8_to_char(res.data, res.len, (char*) tmp + 2);
    res.data = tmp;
    res.len  = res.len * 2 + 2;
  }
  */
  return res;
}

int test_trie(d_token_t* test, uint32_t props, uint64_t* ms) {
  uint64_t   start  = clock();
  trie_t*    trie   = trie_new();
  d_token_t *in     = d_get(test, key("in")), *t, *el;
  uint8_t    is_hex = d_get_int(test, "hexEncoded"), i, tmp[64], tmp2[64], tmp3[32], res = 0;

  if (d_type(in) == T_ARRAY) {
    for (i = 0, t = in + 1; i < d_len(in); i++, t = d_next(t)) {
      bytes_t key_bytes = get_bytes(d_get_at(t, 0), tmp2, is_hex), value_bytes = get_bytes(d_get_at(t, 1), tmp, is_hex);
      if (props & 2) {
        sha3_to(&key_bytes, tmp3);
        key_bytes.data = tmp3;
        key_bytes.len  = 32;
      }
      trie_set_value(trie, &key_bytes, &value_bytes);
      if (props & EVM_PROP_DEBUG) {
        printf("\n\n_____________________\n%i:####### SET ", i + 1);
        ba_print(key_bytes.data, key_bytes.len);
        printf(" = ");
        ba_print(value_bytes.data, value_bytes.len);

        trie_dump(trie, 0);
      }
    }
  } else {

    for (i = 0, t = in + 1; i < d_len(in); i++, t = d_next(t)) {
      char*   k = d_get_keystr(t->key);
      bytes_t key_bytes, value_bytes = get_bytes(t, tmp, is_hex);
      if (k[0] == '0' && k[1] == 'x') {
        key_bytes.data = tmp;
        key_bytes.len  = hex2byte_arr(k + 2, strlen(k) - 2, tmp, 64);
      } else {
        key_bytes.data = (uint8_t*) k;
        key_bytes.len  = strlen(k);
      }

      if (props & 2) {
        sha3_to(&key_bytes, tmp3);
        key_bytes.data = tmp3;
        key_bytes.len  = 32;
      }
      trie_set_value(trie, &key_bytes, &value_bytes);
      if (props & EVM_PROP_DEBUG) {
        printf("\n\n_____________________\n%i:####### SET ", i + 1);
        ba_print(key_bytes.data, key_bytes.len);
        printf(" = ");
        ba_print(value_bytes.data, value_bytes.len);

        trie_dump(trie, 0);
      }
    }
  }
  bytes_t root_bytes = d_to_bytes(d_get(test, key("root")));
  if (root_bytes.len == 32 && memcmp(root_bytes.data, trie->root, 32)) {

    if (props & EVM_PROP_DEBUG) {
      printf("\n expected : ");
      ba_print(root_bytes.data, 32);
      printf("\n       is : ");
      ba_print(trie->root, 32);
      printf("\n");
    }

    print_error("wrong root-hash");
    res = 1;
  }

  trie_free(trie);
  *ms = (clock() - start) / 1000;
  return res;
}