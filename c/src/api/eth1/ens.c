#include "ens.h"
#include "../../core/client/client.h"
#include "../../core/client/context_internal.h"
#include "../../core/client/keys.h"
#include "../../core/util/bytes.h"
#include "../../core/util/data.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include <string.h>

static int next_token(const char* c, int p) {
  for (; p >= 0; p--) {
    if (c[p] == '.') return p;
  }
  return -1;
}

static in3_ctx_t* find_pending_ctx(in3_ctx_t* ctx, bytes_t data) {
  // ok, we need a request, do we have a useable?
  ctx = ctx->required;
  while (ctx) {
    if (strcmp(d_get_stringk(ctx->requests[0], K_METHOD), "eth_call") == 0) {
      bytes_t* ctx_data = d_get_bytesk(d_get_at(d_get(ctx->requests[0], K_PARAMS), 0), K_DATA);
      if (ctx_data && b_cmp(ctx_data, &data)) return ctx;
    }
    ctx = ctx->required;
  }
  return NULL;
}

static in3_ret_t exec_call(bytes_t calldata, char* to, in3_ctx_t* parent, bytes_t** result) {
  in3_ctx_t* ctx = find_pending_ctx(parent, calldata);

  if (ctx) {
    switch (in3_ctx_state(ctx)) {
      case CTX_SUCCESS: {
        d_token_t* rpc_result = d_get(ctx->responses[0], K_RESULT);
        if (!ctx->error && rpc_result && d_type(rpc_result) == T_BYTES && d_len(rpc_result) >= 20) {
          *result = d_bytes(rpc_result);
          //          ctx_remove_required(parent, ctx);
          return IN3_OK;
        } else
          return ctx_set_error(parent, "could not get the resolver", IN3_EFIND);
      }
      case CTX_ERROR:
        return IN3_ERPC;
      default:
        return IN3_WAITING;
    }
  } else {
    // create request
    char* req = _malloc(250);
    char  data[73];
    bytes_to_hex(calldata.data, 36, data);
    sprintf(req, "{\"method\":\"eth_call\",\"jsonrpc\":\"2.0\",\"id\":1,\"params\":[{\"to\":\"%s\",\"data\":\"0x%s\"},\"latest\"]}", to, data);
    return ctx_add_required(parent, ctx_new(parent->client, req));
  }
}

static void ens_hash(const char* domain, bytes32_t dst) {
  uint8_t hash[64];                                                                            // we use the first 32 bytes for the root and the 2nd for the name so we can combine them without copying
  bytes_t input = bytes(NULL, 0), root = bytes(hash, 64);                                      // bytes-strcuts for root
  int     end = strlen(domain);                                                                // we start with the last token
  memset(hash, 0, 32);                                                                         // clear root
  for (int pos = next_token(domain, end - 1);; end = pos, pos = next_token(domain, pos - 1)) { // we start with the last
    input = bytes((uint8_t*) (domain + pos + 1), end - pos - 1);                               // and iterate through the tokens
    sha3_to(&input, hash + 32);                                                                // hash the name
    sha3_to(&root, hash);                                                                      // hash ( root + name )
    if (pos < 0) break;                                                                        //  last one?
  }                                                                                            //
  memcpy(dst, hash, 32);                                                                       // we only the first 32 bytes - the root
}

in3_ret_t ens_resolve(in3_ctx_t* parent, char* name, const address_t registry, in3_ens_type type, uint8_t* dst, int* res_len) {
  const int len = strlen(name);
  if (*name == '0' && name[1] == 'x' && len == 42) {
    hex_to_bytes(name, 40, dst, 20);
    return IN3_OK;
  }

  *res_len = type == ENS_HASH ? 32 : 20;

  char*   cachekey  = NULL;
  bytes_t dst_bytes = bytes(dst, *res_len);

  //check cache
  if (parent->client->cache) {
    cachekey = alloca(strlen(name) + 5);
    sprintf(cachekey, "ens:%s:%i", name, type);
    bytes_t* cached = parent->client->cache->get_item(parent->client->cache->cptr, cachekey);
    if (cached) {
      memcpy(dst, cached->data, 20);
      b_free(cached);
      return IN3_OK;
    }
  }

  uint8_t   calldata[36], *hash = calldata + 4;
  bytes_t   callbytes   = bytes(calldata, 36);
  address_t resolver    = {0};
  bytes_t*  last_result = NULL;
  ens_hash(name, hash);

  if (type == ENS_HASH) {
    memcpy(dst, hash, 32);
    return IN3_OK;
  }

  if (type == ENS_OWNER) {
    // owner(bytes32)
    calldata[0] = 0x02;
    calldata[1] = 0x57;
    calldata[2] = 0x1b;
    calldata[3] = 0xe3;
  } else {
    // resolver(bytes32)
    calldata[0] = 0x01;
    calldata[1] = 0x78;
    calldata[2] = 0xb8;
    calldata[3] = 0xbf;
  }

  // find registry-address
  char* registry_address;
  if (registry) {
    registry_address = alloca(43);
    bytes_to_hex(registry, 20, registry_address + 2);
    registry_address[0] = '0';
    registry_address[1] = 'x';
  } else
    switch (parent->client->chain_id) {
      case ETH_CHAIN_ID_MAINNET:
        registry_address = "0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e";
        break;
      default:
        return ctx_set_error(parent, "There is no ENS-contract for the current chain", IN3_ENOTSUP);
    }

  in3_ret_t res = exec_call(callbytes, registry_address, parent, &last_result);
  if (res < 0) return res;
  if (last_result && last_result->data)
    memcpy(resolver, last_result->data + last_result->len - 20, 20);
  if (memiszero(resolver, 20)) return ctx_set_error(parent, "resolver not registered", IN3_EFIND);

  if (type == ENS_RESOLVER || type == ENS_OWNER) {
    memcpy(dst, resolver, 20);
    if (parent->client->cache)
      parent->client->cache->set_item(parent->client->cache->cptr, cachekey, &dst_bytes);
    return IN3_OK;
  }

  if (type == ENS_ADDR) {
    // now we change the call to addr(bytes32)
    calldata[0] = 0x3b;
    calldata[1] = 0x3b;
    calldata[2] = 0x57;
    calldata[3] = 0xde;
  } else if (type == ENS_NAME) {
    /// name(bytes32) = 0x691f3431f2842c92f
    calldata[0] = 0x69;
    calldata[1] = 0x1f;
    calldata[2] = 0x34;
    calldata[3] = 0x31;
  }

  char r_adr[43];
  bytes_to_hex(resolver, 20, r_adr + 2);
  r_adr[0] = '0';
  r_adr[1] = 'x';

  res = exec_call(callbytes, r_adr, parent, &last_result);
  if (res < 0) return res;
  if (!last_result || !last_result->data) return IN3_ENOMEM;

  if (last_result->len < 20 || memiszero(last_result->data, 20)) return ctx_set_error(parent, "address not registered", IN3_EFIND);

  if (type == ENS_ADDR)
    memcpy(dst, last_result->data + last_result->len - 20, 20);
  else if (type == ENS_NAME) {
  }

  if (parent->client->cache)
    parent->client->cache->set_item(parent->client->cache->cptr, cachekey, &dst_bytes);
  return IN3_OK;
}