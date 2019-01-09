

#include "client/verifier.h"
#include <client/keys.h>
#include <stdio.h>
#include <string.h>

bytes_t* in3_get_code_from_client(in3_vctx_t* vc, char* hex_address) {
  bytes_t*   res = NULL;
  d_token_t* t;
  char       params[100];
  sprintf(params, "[\"0x%s\",\"latest\"]", hex_address + 1);
  in3_ctx_t* ctx = in3_client_rpc_ctx(vc->ctx->client, "eth_getCode", params);
  if (!ctx->error && ctx->responses[0] && (t = d_get(ctx->responses[0], K_RESULT))) {
    if (vc->ctx->client->cacheStorage)
      vc->ctx->client->cacheStorage->set_item(vc->ctx->client->cacheStorage->cptr, hex_address, d_bytes(t));
    else
      res = b_dup(d_bytes(t));
  } else
    vc_err(vc, ctx->error);
  free_ctx(ctx);
  return res;
}

cache_entry_t* in3_get_code(in3_vctx_t* vc, uint8_t* address) {
  for (cache_entry_t* en = vc->ctx->cache; en; en = en->next) {
    if (en->key.len == 20 && memcmp(address, en->key.data, 20) == 0)
      return en;
  }
  char key_str[42];
  key_str[0]  = 'C';
  key_str[41] = 0;
  int8_to_char(address + 1, 20, key_str);
  bytes_t*       b = NULL;
  cache_entry_t* entry;

  // not cached yet
  if (vc->ctx->client->cacheStorage) {
    b = vc->ctx->client->cacheStorage->get_item(vc->ctx->client->cacheStorage->cptr, key_str);
    if (!b) {
      in3_get_code_from_client(vc, key_str);
      b = vc->ctx->client->cacheStorage->get_item(vc->ctx->client->cacheStorage->cptr, key_str);
    }
  } else
    b = in3_get_code_from_client(vc, key_str);

  if (b) {
    bytes_t key = {.len = 20, .data = _malloc(20)};
    memcpy(key.data, address, 20);
    entry          = _malloc(sizeof(cache_entry_t));
    entry->next    = vc->ctx->cache;
    entry->key     = key;
    entry->value   = *b;
    vc->ctx->cache = entry;
    int_to_bytes(b->len, entry->buffer);
    return entry;
  }
  return NULL;
}