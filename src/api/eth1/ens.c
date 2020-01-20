#include "../../core/client/client.h"
#include "../../core/client/context.h"
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
  //  0x0178b8bfbc2fc00072dc46c13002c1694a202596028a76e974308c6f3b6849c0b081f870
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

static in3_ret_t read_address(bytes_t calldata, char* to, in3_ctx_t* parent, address_t dst) {
  in3_ctx_t* ctx = find_pending_ctx(parent, calldata);

  if (ctx) {
    switch (in3_ctx_state(ctx)) {
      case CTX_SUCCESS: {
        d_token_t* rpc_result = d_get(ctx->responses[0], K_RESULT);
        if (!ctx->error && rpc_result && d_type(rpc_result) == T_BYTES && d_len(rpc_result) >= 20) {
          memcpy(dst, rpc_result->data + d_len(rpc_result) - 20, 20);
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

static void ens_hash(char* domain, bytes32_t dst) {
  uint8_t hash[64];
  bytes_t input = bytes(NULL, 0), root = bytes(hash, 64);
  int     end = end = strlen(domain);
  memset(hash, 0, 32);
  for (int pos = next_token(domain, end - 1);; end = pos, pos = next_token(domain, pos - 1)) {
    input = bytes((uint8_t*) (domain + pos + 1), end - pos - 1);
    sha3_to(&input, hash + 32);
    sha3_to(&root, hash);
    if (pos < 0) break;
  }
  memcpy(dst, hash, 32);
}

in3_ret_t ens_resolve_address(in3_ctx_t* parent, char* name, const address_t registry, address_t dst) {
  const int len = strlen(name);
  if (*name == '0' && name[1] == 1 && len == 42) {
    hex_to_bytes(name, 40, dst, 20);
    return IN3_OK;
  }

  //check cache
  if (parent->client->cache) {
    bytes_t* cached = parent->client->cache->get_item(parent->client->cache->cptr, name);
    if (cached) {
      memcpy(dst, cached->data, 20);
      b_free(cached);
      return IN3_OK;
    }
  }

  uint8_t   calldata[36], *hash = calldata + 4;
  bytes_t   callbytes = bytes(calldata, 36);
  address_t resolver;
  ens_hash(name, hash);

  // check the cache
  calldata[0] = 0x01;
  calldata[1] = 0x78;
  calldata[2] = 0xb8;
  calldata[3] = 0xbf;

  // find registry-address
  char* registry_address;
  if (registry) {
    registry_address = alloca(43);
    bytes_to_hex(registry, 20, registry_address + 2);
    registry_address[0] = '0';
    registry_address[0] = 'x';
  } else
    switch (parent->client->chain_id) {
      case ETH_CHAIN_ID_MAINNET:
        registry_address = "0x314159265dD8dbb310642f98f50C066173C1259b";
        break;
      default:
        return ctx_set_error(parent, "There is no ENS-contract for the current chain", IN3_ENOTSUP);
    }

  in3_ret_t res = read_address(callbytes, registry_address, parent, resolver);
  if (res < 0) return res;
  if (memiszero(resolver, 20)) return ctx_set_error(parent, "resolver not registered", IN3_EFIND);

  // now we change the call to addr(bytes32)
  calldata[0] = 0x3b;
  calldata[1] = 0x3b;
  calldata[2] = 0x57;
  calldata[3] = 0xde;

  char r_adr[43];
  bytes_to_hex(resolver, 20, r_adr + 2);
  r_adr[0] = '0';
  r_adr[1] = 'x';

  res = read_address(callbytes, r_adr, parent, dst);
  if (res < 0) return res;
  if (memiszero(resolver, 20)) return ctx_set_error(parent, "address not registered", IN3_EFIND);
  if (parent->client->cache) {
    bytes_t res = bytes(dst, 20);
    parent->client->cache->set_item(parent->client->cache->cptr, name, &res);
  }
  return IN3_OK;
}