#include "btc.h"
#include "../../core/client/keys.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "../../verifier/eth1/nano/eth_nano.h"
#include "btc_merkle.h"
#include "btc_serialize.h"
#include "btc_target.h"
#include "btc_types.h"
#include <stdlib.h>
#include <string.h>

// check if 2 byte arrays are equal where one is a bytes while the other one is a hex string (without 0x)
static bool equals_hex(bytes_t data, char* hex) {
  uint32_t sl = hex ? strlen(hex) : 0, bl = sl >> 1;                                                              // calc len of bytes from hex
  if (bl != data.len || sl % 1) return false;                                                                     // we do not support odd length of hex
  for (uint32_t i = 0; i < bl; i++) {                                                                             // compare each byte
    if (data.data[i] != ((hexchar_to_int(hex[i << 1]) << 4) | (hexchar_to_int(hex[(i << 1) + 1])))) return false; // cancel on first difference
  }
  return true;
}

// check if 2 byte arrays are equal where one is a bytes while the other one is a hex string (without 0x)
static bool equals_hex_rev(bytes_t data, char* hex) {
  uint32_t sl = hex ? strlen(hex) : 0, bl = sl >> 1;                                                                             // calc len of bytes from hex
  if (bl != data.len || sl % 1) return false;                                                                                    // we do not support odd length of hex
  for (uint32_t i = 0; i < bl; i++) {                                                                                            // compare each byte
    if (data.data[data.len - i - 1] != ((hexchar_to_int(hex[i << 1]) << 4) | (hexchar_to_int(hex[(i << 1) + 1])))) return false; // cancel on first difference
  }
  return true;
}

// extract the blocknumber from the proof based on BIP 34
static in3_ret_t btc_block_number(in3_vctx_t* vc, uint32_t* dst_block_number, d_token_t* proof, bytes_t header) {
  bytes_t     merkle_proof = d_to_bytes(d_get(proof, key("cbtxMerkleProof")));
  bytes_t     tx           = d_to_bytes(d_get(proof, key("cbtx")));
  bytes32_t   tx_id;
  btc_tx_t    tx_data;
  btc_tx_in_t tx_in;

  if (header.len != 80) return vc_err(vc, "invalid blockheader");
  if (!merkle_proof.len) return vc_err(vc, "missing merkle proof");
  if (!tx.len) return vc_err(vc, "missing coinbase tx");

  // create txid
  if (btc_parse_tx(tx, &tx_data)) return vc_err(vc, "invalid coinbase tx");
  if (btc_tx_id(&tx_data, tx_id)) return vc_err(vc, "invalid txid!");

  // verify merkle proof
  if (!btc_merkle_verify_proof(btc_block_get(header, BTC_B_MERKLE_ROOT).data, merkle_proof, 0, tx_id)) return vc_err(vc, "merkleProof failed!");

  // the coinbase tx has only one input
  if (tx_data.input_count != 1) return vc_err(vc, "vin count needs to be 1 for coinbase tx");
  if (btc_parse_tx_in(tx_data.input.data, &tx_in, tx_data.input.data + tx_data.input.len) == NULL || *tx_in.script.data != 3) return vc_err(vc, "invalid coinbase signature");

  *dst_block_number = ((uint32_t) tx_in.script.data[1]) | (((uint32_t) tx_in.script.data[2]) << 8) | (((uint32_t) tx_in.script.data[3]) << 16);

  return IN3_OK;
}

/**
 * verify proof of work of the blockheader
 */
static in3_ret_t
btc_verify_header(in3_vctx_t* vc, uint8_t* block_header, bytes32_t dst_hash, bytes32_t dst_target, uint32_t* block_number, bytes32_t expected_target, d_token_t* proof) {
  btc_target_from_block(bytes(block_header, 80), dst_target);                                                                                           // get the target
  btc_hash(bytes(block_header, 80), dst_hash);                                                                                                          // calculate blockhash
  if (memcmp(dst_target, dst_hash, 32) < 0) return vc_err(vc, "Invalid proof of work. the hash is greater than the target");                            // make sure the hash < limit
  if (expected_target && memcmp(dst_target, expected_target, 32)) return vc_err(vc, "Invalid target");                                                  // if expect a target we  check it
  if (block_number && proof && btc_block_number(vc, block_number, proof, bytes(block_header, 80))) return vc_err(vc, "could not get the block number"); // get the blocknumber from proof (BIP-34)
  return IN3_OK;                                                                                                                                        // all is fine
}

/**
 * verify the finality block.
 */
in3_ret_t btc_check_finality(in3_vctx_t* vc, bytes32_t block_hash, int finality, bytes_t final_blocks, bytes32_t expected_target, uint64_t block_nr) {
  if (!finality) return final_blocks.len == 0 ? IN3_OK : vc_err(vc, "gort finalily headers even though they were not expected");
  bytes32_t parent_hash, tmp, target;
  in3_ret_t ret = IN3_OK;
  uint32_t  p   = 0;
  block_nr++;                                                                                                   // we start with the next block_nr
                                                                                                                //
  memcpy(target, expected_target, 32);                                                                          // we start with the expected target, but know it may change if cross the dap
  memcpy(parent_hash, block_hash, 32);                                                                          // we start with the current block hash as parent
  for (int i = 0; i < finality; i++, p += 80, block_nr++) {                                                     // go through all requested finality blocks
    if (p + 80 > (uint32_t) final_blocks.len) return vc_err(vc, "Not enough finality blockheaders");            //  the bytes need to be long enough
    if ((block_nr) % 2016 == 0) {                                                                               // if we reached a epcoch limit
      i = 0;                                                                                                    // we need all finality-headers again startting with the DAP-break.
      btc_target_from_block(bytes(final_blocks.data + p, 80), tmp);                                             // read the new target from the new blockheader
      if ((ret = btc_new_target_check(vc, target, tmp))) return ret;                                            // check if the new target is within the allowed range (*/4)
      memcpy(target, tmp, 32);                                                                                  // now we use the new target
    }                                                                                                           //
    rev_copy(tmp, btc_block_get(bytes(final_blocks.data + p, 80), BTC_B_PARENT_HASH).data);                     // copy the parent hash of the block inito tmp
    if (memcmp(tmp, parent_hash, 32)) return vc_err(vc, "wrong parent_hash in finality block");                 // check parent hash
    if ((ret = btc_verify_header(vc, final_blocks.data + p, parent_hash, tmp, NULL, target, NULL))) return ret; // check the headers proof of work and set the new parent hash
  }

  return final_blocks.len == p ? IN3_OK : vc_err(vc, "too many final headers");
}

in3_ret_t btc_verify_tx(in3_vctx_t* vc, uint8_t* tx_id, bool json, uint8_t* block_hash) {
  bytes_t    data, merkle_data, header, tmp;
  bytes32_t  hash, expected_block_hash, block_target, hash2;
  d_token_t *t, *list;
  btc_tx_t   tx_data;
  bool       in_active_chain = true;
  if (!vc->proof) return vc_err(vc, "missing the proof");

  // define the expected blockhash
  if (block_hash) memcpy(expected_block_hash, block_hash, 32);
  memset(block_target, 0, 32);

  // get the header
  t = d_get(vc->proof, K_BLOCK);
  if (!t || d_type(t) != T_BYTES || d_len(t) != 80) return vc_err(vc, "missing or invalid blockheader!");
  header = d_to_bytes(t);

  if (json) {

    // check txid
    t = d_get(vc->result, key("txid"));
    if (!t || d_type(t) != T_STRING || d_len(t) != 64) return vc_err(vc, "missing or invalid txid");
    hex_to_bytes(d_string(t), 64, hash, 32);
    if (memcmp(hash, tx_id, 32)) return vc_err(vc, "wrong txid");

    // check hex
    t = d_get(vc->result, key("hex"));
    if (!t || d_type(t) != T_STRING) return vc_err(vc, "missing hex");
    data.len  = (d_len(t) + 1) >> 1;
    data.data = _malloc(data.len);
    in3_cache_add_ptr(&vc->ctx->cache, data.data);
    hex_to_bytes(d_string(t), d_len(t), data.data, data.len);

    // parse tx
    if (btc_parse_tx(data, &tx_data)) return vc_err(vc, "invalid tx data");

    // check hash
    t = d_get(vc->result, key("hash"));
    btc_hash(data, hash2);
    if (!t || d_type(t) != T_STRING || d_len(t) != 64) return vc_err(vc, "missing or invalid hash");
    hex_to_bytes(d_string(t), 64, hash, 32);
    if (memcmp(hash, hash2, 32)) return vc_err(vc, "wrong hash");

    // check blockhash
    t = d_get(vc->result, key("blockhash"));
    if (!t || d_type(t) != T_STRING || d_len(t) != 64) return vc_err(vc, "missing blockhash");
    hex_to_bytes(d_string(t), d_len(t), expected_block_hash, 32);
    if (block_hash) {
      if (memcmp(expected_block_hash, block_hash, 32)) return vc_err(vc, "invalid blockhash");
      in_active_chain = d_get_intk(vc->result, key("in_active_chain"));
    }

    // check version
    if (d_get_intk(vc->result, key("version")) != (int32_t) tx_data.version) return vc_err(vc, "invalid version");

    // check size
    if (d_get_intk(vc->result, key("size")) != (int32_t) data.len) return vc_err(vc, "invalid size");

    // check vsize
    if (d_get_intk(vc->result, key("vsize")) != (int32_t) btc_vsize(&tx_data)) return vc_err(vc, "invalid vsize");

    // weight
    if (d_get_intk(vc->result, key("weight")) != (int32_t) btc_weight(&tx_data)) return vc_err(vc, "invalid weight");

    // locktime
    if (d_get_longk(vc->result, key("locktime")) != tx_data.lock_time) return vc_err(vc, "invalid locktime");

    // blocktime
    if (!d_eq(d_get(vc->result, key("time")), d_get(vc->result, key("blocktime")))) return vc_err(vc, "invalid blocktime");

    // time
    tmp = btc_block_get(header, BTC_B_TIMESTAMP);
    if (tmp.len == 4 && le_to_int(tmp.data) != (uint32_t) d_get_longk(vc->result, key("time"))) return vc_err(vc, "invalid time");

    // check vin
    uint8_t*    p        = tx_data.input.data;
    uint8_t*    end      = p + tx_data.input.len;
    uint32_t    tx_index = d_get_intk(vc->proof, key("txIndex"));
    btc_tx_in_t tx_in;
    char*       hex;
    list = d_get(vc->result, key("vin"));
    if (d_type(list) != T_ARRAY || d_len(list) != (int) tx_data.input_count) return vc_err(vc, "invalid vin");

    for (d_iterator_t iter = d_iter(list); iter.left; d_iter_next(&iter)) {
      p = btc_parse_tx_in(p, &tx_in, end);
      if (!p) return vc_err(vc, "invalid vin");

      // sequence
      if (d_get_longk(iter.token, key("sequence")) != tx_in.sequence) return vc_err(vc, "invalid vin.sequence");

      if (tx_index == 0) {
        // coinbase
        hex = d_get_stringk(iter.token, key("coinbase"));
        if (!hex || !equals_hex(tx_in.script, hex)) return vc_err(vc, "invalid coinbase");
      } else {
        // txid
        hex = d_get_stringk(iter.token, key("txid"));
        if (!equals_hex_rev(bytes(tx_in.prev_tx_hash, 32), hex)) return vc_err(vc, "invalid vin.txid");

        // vout
        if (d_get_intk(iter.token, key("vout")) != (int32_t) tx_in.prev_tx_index) return vc_err(vc, "invalid vin.vout");

        // sig.hex
        hex = d_get_stringk(d_get(iter.token, key("scriptSig")), key("hex"));
        if (!equals_hex(tx_in.script, hex)) return vc_err(vc, "invalid vin.hex");
      }
    }

    p   = tx_data.output.data;
    end = p + tx_data.output.len;
    btc_tx_out_t tx_out;
    int32_t      n = 0;
    list           = d_get(vc->result, key("vout"));
    if (d_type(list) != T_ARRAY || d_len(list) != (int) tx_data.output_count) return vc_err(vc, "invalid vout");

    for (d_iterator_t iter = d_iter(list); iter.left; d_iter_next(&iter), n++) {
      p = btc_parse_tx_out(p, &tx_out);
      if (p > end) return vc_err(vc, "invalid vout");

      // n
      if (d_get_intk(iter.token, key("n")) != n) return vc_err(vc, "invalid vout.n");

      // sig.hex
      char* hex = d_get_stringk(d_get(iter.token, key("scriptPubKey")), key("hex"));
      if (!equals_hex(tx_out.script, hex)) return vc_err(vc, "invalid vout.hex");
      d_token_t* value = d_get(iter.token, K_VALUE);
      if (!value) return vc_err(vc, "no value found!");

      if (d_type(value) == T_STRING) {
        if (parse_float_val(d_string(value), 8) != (int64_t) tx_out.value)
          return vc_err(vc, "wrong value in txout found!");
      } else if (d_type(value) == T_INTEGER || d_type(value) == T_BYTES) {
        if (d_long(value) * 10e8 != tx_out.value)
          return vc_err(vc, "wrong value in txout found!");
      } else
        return vc_err(vc, "wrong type of value!");
    }

  } else {

    // here we expect the raw serialized transaction
    if (!vc->result || d_type(vc->result) != T_STRING) return vc_err(vc, "expected hex-data as result");
    data.len  = (d_len(vc->result) + 1) >> 1;
    data.data = _malloc(data.len);
    in3_cache_add_ptr(&vc->ctx->cache, data.data);
    hex_to_bytes(d_string(vc->result), d_len(vc->result), data.data, data.len);

    // parse tx
    if (btc_parse_tx(data, &tx_data)) return vc_err(vc, "invalid tx data");
  }

  // hash and check transaction hash
  if (btc_tx_id(&tx_data, hash)) return vc_err(vc, "invalid txdata");
  if (memcmp(hash, tx_id, 32)) return vc_err(vc, "invalid txid");

  // now check the merkle proof
  t = d_get(vc->proof, K_MERKLE_PROOF);
  if (!t || d_type(t) != T_BYTES) return vc_err(vc, "missing merkle proof!");
  merkle_data = d_to_bytes(t);

  // check the transactionIndex
  t = d_get(vc->proof, K_TX_INDEX);
  if (!t || d_type(t) != T_INTEGER) return vc_err(vc, "missing txIndex");

  // verify merkle proof
  if (!btc_merkle_verify_proof(btc_block_get(header, BTC_B_MERKLE_ROOT).data, merkle_data, d_int(t), tx_id)) return vc_err(vc, "merkleProof failed!");

  // now verify the blockheader including finality and target
  uint32_t  block_number     = 0;
  bytes_t   finality_headers = d_to_bytes(d_get(vc->proof, key("final")));
  in3_ret_t ret;

  if ((ret = btc_verify_header(vc, header.data, hash, block_target, &block_number, NULL, vc->proof))) return ret;
  if ((block_hash || json) && memcmp(expected_block_hash, hash, 32)) return vc_err(vc, "invalid hash of blockheader!");
  if (!in_active_chain) return IN3_OK;
  if ((ret = btc_check_finality(vc, hash, vc->client->finality, finality_headers, block_target, block_number))) return ret;
  if ((ret = btc_check_target(vc, block_number, block_target, finality_headers, header))) return ret;

  return IN3_OK;
}

/**
 * check a block
 */
in3_ret_t btc_verify_blockcount(in3_vctx_t* vc) {
  // verify the blockheader
  //TODO verify the proof
  return vc->proof ? IN3_OK : vc_err(vc, "missing the proof");
}

/**
 * check a block
 */
in3_ret_t btc_verify_block(in3_vctx_t* vc, bytes32_t block_hash, int verbose, bool full_block) {
  uint8_t   block_header[80];
  bytes32_t block_target, hash, tmp, tmp2;
  in3_ret_t ret              = IN3_OK;
  uint32_t  block_number     = 0;
  bytes_t   finality_headers = d_to_bytes(d_get(vc->proof, key("final")));
  if (!vc->proof) return vc_err(vc, "missing the proof");
  if (verbose)
    btc_serialize_block_header(vc->result, block_header);      // we need to serialize the header first, so we can check the hash
  else                                                         //
    hex_to_bytes(d_string(vc->result), 160, block_header, 80); // or use the first 80 byte of the block

  // verify the blockheader
  if ((ret = btc_verify_header(vc, block_header, hash, block_target, &block_number, NULL, vc->proof))) return ret;
  if ((ret = btc_check_finality(vc, hash, vc->client->finality, finality_headers, block_target, block_number))) return ret;
  if ((ret = btc_check_target(vc, block_number, block_target, finality_headers, bytes(block_header, 80)))) return ret;

  // check blockhash
  if (memcmp(hash, block_hash, 32)) return vc_err(vc, "Invalid blockhash");

  // check the transactions
  if (full_block) {
    // check merkleroot
    if (verbose) {
      d_token_t* tx       = d_get(vc->result, key("tx"));                                                                             // get transactions node
      int        tx_count = d_len(tx), i = 0;                                                                                         // and count its length
      bytes32_t* tx_hashes = _malloc(tx_count * sizeof(bytes32_t));                                                                   // to reserve hashes-array
      for (d_iterator_t iter = d_iter(tx); iter.left; d_iter_next(&iter), i++)                                                        // iterate through all txs
        hex_to_bytes(verbose == 1 ? d_string(iter.token) : d_get_stringk(iter.token, key("txid")), 64, tx_hashes[i], 32);             // and copy the hash into the array
      btc_merkle_create_root(tx_hashes, tx_count, tmp);                                                                               // calculate the merkle root
      _free(tx_hashes);                                                                                                               // cleanup
      rev_copy(tmp2, tmp);                                                                                                            // we need to turn it into little endian be cause ini the header it is store as le.
      if (memcmp(tmp2, btc_block_get(bytes(block_header, 80), BTC_B_MERKLE_ROOT).data, 32)) return vc_err(vc, "Invalid Merkle root"); // compare the hash
                                                                                                                                      //
      btc_target_from_block(bytes(block_header, 80), tmp2);                                                                           // get current target
      uint64_t difficulty = 0xFFFF000000000000L / bytes_to_long(tmp2 + 4, 8);                                                         // and calc the difficulty
      if (difficulty >> 2 != d_get_long(vc->result, "difficulty") >> 2) return vc_err(vc, "Wrong difficulty");                        // which must match the one in the json
      if (!equals_hex(bytes(block_hash, 32), d_get_string(vc->result, "hash"))) return vc_err(vc, "Wrong blockhash in json");         // check the requested hash
      if (d_get_int(vc->result, "nTx") != (int32_t) tx_count) return vc_err(vc, "Wrong nTx");                                         // check the nuumber of transactions

    } else {
      char*    block_hex  = d_string(vc->result);
      uint8_t* block_data = _malloc(strlen(block_hex) / 2);
      bytes_t  block      = bytes(block_data, strlen(block_hex) / 2);
      hex_to_bytes(block_hex, -1, block.data, block.len);
      int        tx_count     = btc_get_transaction_count(block);
      bytes_t*   transactions = _malloc(tx_count * sizeof(bytes_t));
      bytes32_t* tx_hashes    = _malloc(tx_count * sizeof(bytes32_t));
      btc_get_transactions(block, transactions);

      // now calculate the transactionhashes
      btc_tx_t tx;
      for (int i = 0; i < tx_count; i++) {
        btc_parse_tx(transactions[i], &tx);
        btc_tx_id(&tx, (uint8_t*) (tx_hashes + i));
      }
      btc_merkle_create_root(tx_hashes, tx_count, tmp);
      rev_copy(tmp2, tmp);
      in3_ret_t res = (memcmp(tmp2, btc_block_get(block, BTC_B_MERKLE_ROOT).data, 32)) ? vc_err(vc, "Invalid Merkle root") : IN3_OK;
      _free(block_data);
      _free(transactions);
      _free(tx_hashes);
      if (res) return res;
    }
  }

  // check other properties
  if (verbose) {
    // check nextblockhash
    if (finality_headers.len) {
      btc_hash(bytes(finality_headers.data, 80), hash);
      if (!equals_hex(bytes(hash, 32), d_get_stringk(vc->result, key("nextblockhash"))))
        return vc_err(vc, "Invalid nextblockhash");
    }

    if (*block_header != d_get_intk(vc->request, key("version"))) return vc_err(vc, "Invalid version");
  }

  return IN3_OK;
}

in3_ret_t btc_verify_target_proof(in3_vctx_t* vc, d_token_t* params) {
  if (d_len(params) != 5) return vc_err(vc, "must have 5 params!");
  bytes32_t          hash, block_target;
  in3_ret_t          ret          = IN3_OK;
  btc_target_conf_t* conf         = btc_get_config(vc);
  uint32_t           block_number = 0;
  if (conf->max_diff != (uint_fast16_t) d_get_int_at(params, 2)) return vc_err(vc, "invalid max_diff");
  if (conf->max_daps != (uint_fast16_t) d_get_int_at(params, 3)) return vc_err(vc, "invalid max_daps");
  //  if (conf->dap_limit != (uint_fast16_t) d_get_int_at(params, 4)) return vc_err(vc, "invalid dap_limit");

  for (d_iterator_t iter = d_iter(vc->result); iter.left; d_iter_next(&iter)) {
    if (d_type(iter.token) != T_OBJECT) return vc_err(vc, "invalid type for proof");
    bytes_t header           = d_to_bytes(d_get(iter.token, K_BLOCK));
    bytes_t finality_headers = d_to_bytes(d_get(iter.token, key("final")));
    if (header.len != 80) return vc_err(vc, "invalid header");

    if ((ret = btc_verify_header(vc, header.data, hash, block_target, &block_number, NULL, iter.token))) return ret;
    if ((ret = btc_check_finality(vc, hash, vc->client->finality, finality_headers, block_target, block_number))) return ret;
    if ((ret = btc_check_target(vc, block_number, block_target, finality_headers, header))) return ret;
  }

  return IN3_OK;
}

in3_ret_t in3_verify_btc(void* pdata, in3_plugin_act_t action, void* pctx) {
  UNUSED_VAR(pdata);
  if (action == PLGN_ACT_TERM) {
    in3_t* c = pctx;
    for (int i = 0; i < c->chains_length; i++) {
      if (c->chains[i].type == CHAIN_BTC && c->chains[i].conf) {
        btc_target_conf_t* tc = c->chains[i].conf;
        if (tc->data.data) _free(tc->data.data);
        _free(tc);
      }
    }
    return IN3_OK;
  }
  if (action != PLGN_ACT_RPC_VERIFY) return IN3_EIGNORE;
  in3_vctx_t* vc     = pctx;
  char*       method = d_get_stringk(vc->request, K_METHOD);
  d_token_t*  params = d_get(vc->request, K_PARAMS);
  bytes32_t   hash;
  // we only verify BTC
  if (vc->chain->type != CHAIN_BTC) return IN3_EIGNORE;

  // make sure we want to verify
  if (in3_ctx_get_proof(vc->ctx, vc->index) == PROOF_NONE) return IN3_OK;

  // do we support this request?
  if (!method) return vc_err(vc, "No Method in request defined!");

  // do we have a result? if not it is a vaslid error-response
  if (!vc->result || d_type(vc->result) == T_NULL) return IN3_OK;

  if (strcmp(method, "getblock") == 0) {
    d_token_t* block_hash = d_get_at(params, 0);
    if (d_len(params) < 1 || d_type(params) != T_ARRAY || d_type(block_hash) != T_STRING || d_len(block_hash) != 64) return vc_err(vc, "Invalid params");
    hex_to_bytes(d_string(block_hash), 64, hash, 32);
    return btc_verify_block(vc, hash, d_len(params) > 1 ? d_get_int_at(params, 1) : 1, true);
  }
  if (strcmp(method, "getblockcount") == 0) {
    return btc_verify_blockcount(vc);
  }
  if (strcmp(method, "getblockheader") == 0) {
    d_token_t* block_hash = d_get_at(params, 0);
    if (d_len(params) < 1 || d_type(params) != T_ARRAY || d_type(block_hash) != T_STRING || d_len(block_hash) != 64) return vc_err(vc, "Invalid blockhash");
    hex_to_bytes(d_string(block_hash), 64, hash, 32);
    return btc_verify_block(vc, hash, d_len(params) > 1 ? d_get_int_at(params, 1) : 1, false);
  }
  if (strcmp(method, "in3_proofTarget") == 0) {
    return btc_verify_target_proof(vc, params);
  }
  if (strcmp(method, "getrawtransaction") == 0) {
    d_token_t* tx_id      = d_get_at(params, 0);
    bool       json       = d_len(params) < 2 ? true : d_get_int_at(params, 1);
    d_token_t* block_hash = d_get_at(params, 2);
    if (!tx_id || d_type(tx_id) != T_STRING || d_len(tx_id) != 64) return vc_err(vc, "Invalid tx_id");
    bytes32_t tx_hash_bytes;
    hex_to_bytes(d_string(tx_id), 64, tx_hash_bytes, 32);
    if (block_hash) hex_to_bytes(d_string(block_hash), 64, hash, 32);
    return btc_verify_tx(vc, tx_hash_bytes, json, block_hash ? hash : NULL);
  }
  return IN3_EIGNORE;
}
in3_ret_t in3_register_btc(in3_t* c) {
  in3_register_eth_nano(c);
  return in3_plugin_register(c, PLGN_ACT_RPC_VERIFY || PLGN_ACT_TERM, in3_verify_btc, NULL, false);
}
/*
void in3_register_btc() {
  in3_verifier_t* v = _calloc(1, sizeof(in3_verifier_t));
  v->type           = CHAIN_BTC;
  v->pre_handle     = btc_handle_intern;
  v->verify         = in3_verify_btc;
  v->set_confg      = btc_vc_set_config;
  v->free_chain     = btc_vc_free;
  in3_register_verifier(v);
}
*/
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

*/