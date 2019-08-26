#include "btc.h"
#include "../../core/client/keys.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "btc_merkle.h"
#include "btc_serialize.h"
#include <stdlib.h>
#include <string.h>

static bool equals_hex(bytes_t data, char* hex) {
  int sl = hex ? strlen(hex) : 0, bl = sl >> 1;
  if (bl != (int) data.len || sl % 1) return false;
  for (int i = 0; i < bl; i++) {
    if (data.data[i] != ((strtohex(hex[i << 1]) << 4) | (strtohex(hex[(i << 1) + 1])))) return false;
  }
  return true;
}

in3_ret_t btc_handle_intern(in3_ctx_t* ctx, in3_response_t** response) {
  if (ctx->len > 1 || !response) return IN3_ENOTSUP; // internal handling is only possible for single requests (at least for now)

  // d_token_t* req = ctx->requests[0];
  // check method
  //  if (strcmp(d_get_stringk(req, K_METHOD), "eth_sendTransaction") == 0)
  return IN3_OK;
}

static in3_ret_t btc_verify_header(in3_vctx_t* vc, uint8_t* block_header, bytes32_t dst_hash) {
  bytes32_t target;

  // check target
  btc_target_le(bytes(block_header, 80), dst_hash);
  rev_copy(target, dst_hash);

  // check blockhash
  btc_hash(bytes(block_header, 80), dst_hash);
  if (memcmp(target, dst_hash, 32) < 0) return vc_err(vc, "Invalid proof of work. the hash is greater than the target");

  return IN3_OK;
}

in3_ret_t btc_check_finality(in3_vctx_t* vc, bytes32_t block_hash, int finality, bytes_t final_blocks) {
  bytes32_t parent_hash, tmp;
  in3_ret_t ret = IN3_OK;
  memcpy(parent_hash, block_hash, 32);
  for (int i = 0, p = 0; i < finality; i++, p += 80) {
    if (p + 80 > (int) final_blocks.len) return vc_err(vc, "Not enough finality blockheaders");
    rev_copy(tmp, btc_block_get(bytes(final_blocks.data + p, 80), BTC_B_PARENT_HASH).data);
    if (memcmp(tmp, parent_hash, 32)) return vc_err(vc, "wrong parent_hash in finality block");
    if ((ret = btc_verify_header(vc, final_blocks.data + p, parent_hash))) return ret;
  }
  return ret;
}

in3_ret_t btc_verify_block(in3_vctx_t* vc, bytes32_t block_hash, bool json) {
  uint8_t   block_header[80];
  bytes32_t tmp, tmp2;
  in3_ret_t ret = IN3_OK;
  if (json)
    btc_serialize_block_header(vc->result, block_header);
  else
    hex2byte_arr(d_string(vc->result), 160, block_header, 80);

  // verify the proof of work
  if ((ret = btc_verify_header(vc, block_header, tmp))) return ret;

  // check blockhash
  if (memcmp(tmp, block_hash, 32)) return vc_err(vc, "Invalid blockhash");

  // check merkleroot
  if (json) {
    d_token_t* tx       = d_get(vc->result, key("tx"));
    int        tx_count = d_len(tx), i = 0;
    bytes32_t  tx_hashes[tx_count];
    for (d_iterator_t iter = d_iter(tx); iter.left; d_iter_next(&iter), i++)
      hex2byte_arr(d_string(iter.token), 64, tx_hashes[i], 32);
    btc_merkle_create_root(tx_hashes, tx_count, tmp);
    rev_copy(tmp2, tmp);
    if (memcmp(tmp2, btc_block_get(bytes(block_header, 80), BTC_B_MERKLE_ROOT).data, 32)) return vc_err(vc, "Invalid Merkle root");

    btc_target_le(bytes(block_header, 80), tmp); // current target
    rev_copy(tmp2, tmp);
    uint64_t difficulty = 0xFFFF000000000000L / bytes_to_long(tmp2 + 4, 8);
    if (difficulty >> 2 != d_get_long(vc->result, "difficulty") >> 2) return vc_err(vc, "Wrong difficulty");

    // verify json-data
    if (!equals_hex(bytes(block_hash, 32), d_get_string(vc->result, "hash"))) return vc_err(vc, "Wrong blockhash in json");
    if (d_get_int(vc->result, "nTx") != (uint32_t) tx_count) return vc_err(vc, "Wrong nTx");

  } else {
    char*   block_hex = d_string(vc->result);
    uint8_t block_data[strlen(block_hex) / 2];
    bytes_t block = bytes(block_data, strlen(block_hex) / 2);
    hex2byte_arr(block_hex, -1, block.data, block.len);
    int       tx_count = btc_get_transaction_count(block);
    bytes_t   transactions[tx_count];
    bytes32_t tx_hashes[tx_count];
    btc_get_transactions(block, transactions);

    // now calculate the transactionhashes
    for (int i = 0; i < tx_count; i++)
      btc_hash(transactions[i], tx_hashes[i]);
    btc_merkle_create_root(tx_hashes, tx_count, tmp);
    rev_copy(tmp2, tmp);
    if (memcmp(tmp2, btc_block_get(block, BTC_B_MERKLE_ROOT).data, 32)) return vc_err(vc, "Invalid Merkle root");
  }

  // check finality blocks
  if (vc->ctx->requests_configs->finality)
    return btc_check_finality(vc, block_hash, vc->ctx->requests_configs->finality, d_to_bytes(d_get(vc->proof, key("final"))));

  return IN3_OK;
}

in3_ret_t in3_verify_transaction(in3_vctx_t* vc, bytes32_t transaction_hash, bool json) {
}

in3_ret_t in3_verify_btc(in3_vctx_t* vc) {
  char*      method = d_get_stringk(vc->request, K_METHOD);
  d_token_t* params = d_get(vc->request, K_PARAMS);
  bytes32_t  hash;

  // make sure we want to verify
  if (vc->config->verification == VERIFICATION_NEVER) return IN3_OK;

  // do we have a result? if not it is a vaslid error-response
  if (!vc->result || d_type(vc->result) == T_NULL) return IN3_OK;

  // do we support this request?
  if (!method) return vc_err(vc, "No Method in request defined!");

  if (strcmp(method, "getblock") == 0) {
    d_token_t* block_hash = d_get_at(params, 0);
    if (d_len(params) < 1 || d_type(params) != T_ARRAY || d_type(block_hash) != T_STRING || d_len(block_hash) != 64) return vc_err(vc, "Invalid params");
    hex2byte_arr(d_string(block_hash), 64, hash, 32);
    return btc_verify_block(vc, hash, d_len(params) > 1 ? d_get_int_at(params, 1) : 1);
  }
  return vc_err(vc, "Unsupported method");
}

void in3_register_btc() {
  in3_verifier_t* v = _calloc(1, sizeof(in3_verifier_t));
  v->type           = CHAIN_BTC;
  v->pre_handle     = btc_handle_intern;
  v->verify         = in3_verify_btc;
  in3_register_verifier(v);
}
/*
static void print_hex(char* prefix, uint8_t* data, int len) {
  printf("%s0x", prefix);
  for (int i = 0; i < len; i++) printf("%02x", data[i]);
  printf("\n");
}
static void print(char* prefix, bytes_t data, char* type) {
  uint8_t tmp[32];
  if (strcmp(type, "hash") == 0) {
    rev_copy(tmp, data.data);
    data.data = tmp;
  }

  if (strcmp(type, "int") == 0) {
    printf("%s%i\n", prefix, le_to_int(data.data));
    return;
  }

  print_hex(prefix, data.data, data.len);
}

int main() {

  char* block_hex = BLOCK_B;

  uint8_t block_data[strlen(block_hex) / 2];
  bytes_t block = bytes(block_data, strlen(block_hex) / 2);
  hex2byte_arr(block_hex, -1, block.data, block.len);

  print("version    = ", btc_block_get(block, BTC_B_VERSION), "hex");
  print("parent_hash= ", btc_block_get(block, BTC_B_PARENT_HASH), "hash");
  print("merkle_root= ", btc_block_get(block, BTC_B_MERKLE_ROOT), "hash");
  print("timestamp  = ", btc_block_get(block, BTC_B_TIMESTAMP), "int");
  print("bits       = ", btc_block_get(block, BTC_B_BITS), "hex");
  print("nonce      = ", btc_block_get(block, BTC_B_NONCE), "hex");

  bytes32_t hash;
  btc_hash(btc_block_get(block, BTC_B_HEADER), hash);
  print_hex("hash       = ", hash, 32);
  print("header     = ", btc_block_get(block, BTC_B_HEADER), "hex");

  btc_target_le(block, hash);
  print("target     = ", bytes(hash, 32), "hash");

  int tx_count = btc_get_transaction_count(block);
  printf("tx_cnt     = %i\n", tx_count);
  bytes_t transactions[tx_count];
  btc_get_transactions(block, transactions);

  bytes32_t tx_hashes[tx_count];

  // now calculate the transactionhashes
  for (int i = 0; i < tx_count; i++) {
    btc_hash(transactions[i], tx_hashes[i]);
    printf(" %i : ", i);
    print("", bytes(tx_hashes[i], 32), "hex");
  }

  bytes32_t root;
  btc_merkle_create_root(tx_hashes, tx_count, root);
  print("root = ", bytes(root, 32), "hex");

  int proof_index = 3;

  bytes_t* proof = btc_merkle_create_proof(tx_hashes, tx_count, proof_index);
  print("proof= ", *proof, "hex");
  int verified = btc_merkle_verify_proof(root, *proof, proof_index, tx_hashes[proof_index]);

  printf("VERIFIED : %i\n", verified);

  //  print("rest \n", bytes(block.data + 80, block.len - 80), "hex");
}
*/