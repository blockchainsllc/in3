#include "btc.h"
#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/debug.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "../../verifier/eth1/nano/eth_nano.h"
#include "btc_address.h"
#include "btc_merkle.h"
#include "btc_serialize.h"
#include "btc_sign.h"
#include "btc_target.h"
#include "btc_types.h"
#include <stdlib.h>
#include <string.h>
#ifdef BTC_PRE_BPI34
#include "pre_bip34.h"
static in3_ret_t check_pre_bip34(in3_vctx_t* vc, bytes_t finality_headers, uint64_t bn) {
  bytes32_t blockhash;
  uint64_t  checkpoint     = bn / PRE_BIP34_DISTANCE + 1;
  uint64_t  p              = checkpoint * PRE_BIP34_DISTANCE - bn;
  uint8_t   start_hash[16] = {0};
  if (bn > BIP34_START) return vc_err(vc, "block needs to support BIP34");
  if (checkpoint * 12 + 12 >= btc_pre_bip34_len) return vc_err(vc, "Blocknumber not before bip34");
  memcpy(start_hash + 4, btc_pre_bip34 + checkpoint * 12 - 12, 12);
  if (finality_headers.len < p * 80) return vc_err(vc, "Not enough fnialiity headers");
  btc_hash(bytes(finality_headers.data + (p - 1) * 80, 80), blockhash);
  if (memcmp(blockhash, start_hash, 16)) return vc_err(vc, "invalid finality header");
  return IN3_OK;
}

#endif

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
  bytes_t     merkle_proof = d_bytes(d_get(proof, key("cbtxMerkleProof")));
  bytes_t     tx           = d_bytes(d_get(proof, key("cbtx")));
  bytes32_t   tx_id;
  btc_tx_t    tx_data;
  btc_tx_in_t tx_in;

  if (*header.data == 1 && memiszero(header.data + 1, 3)) {
    *dst_block_number = (uint32_t) d_get_int(proof, key("height"));
    if (!*dst_block_number) return vc_err(vc, "missing height in proof for blocks pre bip34");
#ifdef BTC_PRE_BPI34
    return check_pre_bip34(vc, d_bytes(d_get(proof, key("final"))), *dst_block_number);
#else
    return vc_err(vc, "no pre bip34 support");
#endif
  }
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
  if (btc_parse_tx_in(tx_data.input.data, &tx_in, tx_data.input.data + tx_data.input.len) == NULL || *tx_in.script.data.data != 3) return vc_err(vc, "invalid coinbase signature");

  *dst_block_number = ((uint32_t) tx_in.script.data.data[1]) | (((uint32_t) tx_in.script.data.data[2]) << 8) | (((uint32_t) tx_in.script.data.data[3]) << 16);

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
  if (!finality) return final_blocks.len == 0 ? IN3_OK : vc_err(vc, "got finalily headers even though they were not expected");
  bytes32_t parent_hash, tmp, target;
  in3_ret_t ret = IN3_OK;
  uint32_t  p   = 0;
  if (block_nr <= BIP34_START) finality = max(finality, (int) final_blocks.len / 80);                           // for pre bip34 we take all headers, because btc_blocknumber already checked the length
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

  return final_blocks.len >= p ? IN3_OK : vc_err(vc, "too many final headers");
}

in3_ret_t btc_verify_tx(btc_target_conf_t* conf, in3_vctx_t* vc, uint8_t* tx_id, bool json, uint8_t* block_hash) {
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
  t      = d_get(vc->proof, K_BLOCK);
  header = d_bytes(t);
  if (!t || d_type(t) != T_BYTES || d_len(t) != 80) return vc_err(vc, "missing or invalid blockheader!");

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
    in3_cache_add_ptr(&vc->req->cache, data.data);
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
      in_active_chain = d_get_int(vc->result, key("in_active_chain"));
    }

    // check version
    if (d_get_int(vc->result, key("version")) != (int32_t) tx_data.version) return vc_err(vc, "invalid version");

    // check size
    if (d_get_int(vc->result, key("size")) != (int32_t) data.len) return vc_err(vc, "invalid size");

    // check vsize
    if (d_get_int(vc->result, key("vsize")) != (int32_t) btc_vsize(&tx_data)) return vc_err(vc, "invalid vsize");

    // weight
    if (d_get_int(vc->result, key("weight")) != (int32_t) btc_weight(&tx_data)) return vc_err(vc, "invalid weight");

    // locktime
    if (d_get_long(vc->result, key("locktime")) != tx_data.lock_time) return vc_err(vc, "invalid locktime");

    // blocktime
    if (!d_eq(d_get(vc->result, key("time")), d_get(vc->result, key("blocktime")))) return vc_err(vc, "invalid blocktime");

    // time
    tmp = btc_block_get(header, BTC_B_TIMESTAMP);
    if (tmp.len == 4 && le_to_int(tmp.data) != (uint32_t) d_get_long(vc->result, key("time"))) return vc_err(vc, "invalid time");

    // check vin
    uint8_t*    p        = tx_data.input.data;
    uint8_t*    end      = p + tx_data.input.len;
    uint32_t    tx_index = d_get_int(vc->proof, key("txIndex"));
    btc_tx_in_t tx_in;
    char*       hex;
    list = d_get(vc->result, key("vin"));
    if (d_type(list) != T_ARRAY || d_len(list) != (int) tx_data.input_count) return vc_err(vc, "invalid vin");

    for (d_iterator_t iter = d_iter(list); iter.left; d_iter_next(&iter)) {
      p = btc_parse_tx_in(p, &tx_in, end);
      if (!p) return vc_err(vc, "invalid vin");

      // sequence
      if (d_get_long(iter.token, key("sequence")) != tx_in.sequence) return vc_err(vc, "invalid vin.sequence");

      if (tx_index == 0) {
        // coinbase
        hex = d_get_string(iter.token, key("coinbase"));
        if (!hex || !equals_hex(tx_in.script.data, hex)) return vc_err(vc, "invalid coinbase");
      }
      else {
        // txid
        hex = d_get_string(iter.token, key("txid"));
        if (!equals_hex_rev(bytes(tx_in.prev_tx_hash, 32), hex)) return vc_err(vc, "invalid vin.txid");

        // vout
        if (d_get_int(iter.token, key("vout")) != (int32_t) tx_in.prev_tx_index) return vc_err(vc, "invalid vin.vout");

        // sig.hex
        hex = d_get_string(d_get(iter.token, key("scriptSig")), key("hex"));
        if (!equals_hex(tx_in.script.data, hex)) return vc_err(vc, "invalid vin.hex");
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
      if (d_get_int(iter.token, key("n")) != n) return vc_err(vc, "invalid vout.n");

      // sig.hex
      char* hex = d_get_string(d_get(iter.token, key("scriptPubKey")), key("hex"));
      if (!equals_hex(tx_out.script.data, hex)) return vc_err(vc, "invalid vout.hex");
      d_token_t* value = d_get(iter.token, K_VALUE);
      if (!value) return vc_err(vc, "no value found!");

      if (d_type(value) == T_STRING) {
        if (parse_float_val(d_string(value), 8) != (int64_t) tx_out.value)
          return vc_err(vc, "wrong value in txout found!");
      }
      else if (d_type(value) == T_INTEGER || d_type(value) == T_BYTES) {
        if (d_long(value) * 10e8 != tx_out.value)
          return vc_err(vc, "wrong value in txout found!");
      }
      else
        return vc_err(vc, "wrong type of value!");
    }
  }
  else {

    // here we expect the raw serialized transaction
    if (!vc->result || d_type(vc->result) != T_STRING) return vc_err(vc, "expected hex-data as result");
    data.len  = (d_len(vc->result) + 1) >> 1;
    data.data = _malloc(data.len);
    in3_cache_add_ptr(&vc->req->cache, data.data);
    hex_to_bytes(d_string(vc->result), d_len(vc->result), data.data, data.len);

    // parse tx
    if (btc_parse_tx(data, &tx_data)) return vc_err(vc, "invalid tx data");
  }

  // hash and check transaction hash
  if (btc_tx_id(&tx_data, hash)) return vc_err(vc, "invalid txdata");
  if (memcmp(hash, tx_id, 32)) return vc_err(vc, "invalid txid");

  // now check the merkle proof
  t           = d_get(vc->proof, K_MERKLE_PROOF);
  merkle_data = d_bytes(t);
  if (!t || d_type(t) != T_BYTES) return vc_err(vc, "missing merkle proof!");

  // check the transactionIndex
  t = d_get(vc->proof, K_TX_INDEX);
  if (!t || d_type(t) != T_INTEGER) return vc_err(vc, "missing txIndex");

  // verify merkle proof
  if (!btc_merkle_verify_proof(btc_block_get(header, BTC_B_MERKLE_ROOT).data, merkle_data, d_int(t), tx_id)) return vc_err(vc, "merkleProof failed!");

  // now verify the blockheader including finality and target
  uint32_t  block_number     = 0;
  bytes_t   finality_headers = d_bytes(d_get(vc->proof, key("final")));
  in3_ret_t ret;

  if ((ret = btc_verify_header(vc, header.data, hash, block_target, &block_number, NULL, vc->proof))) return ret;
  if ((block_hash || json) && memcmp(expected_block_hash, hash, 32)) return vc_err(vc, "invalid hash of blockheader!");
  if (!in_active_chain) return IN3_OK;
  if ((ret = btc_check_finality(vc, hash, vc->client->finality, finality_headers, block_target, block_number))) return ret;
  if ((ret = btc_check_target(conf, vc, block_number, block_target, finality_headers, header))) return ret;

  return IN3_OK;
}

/**
 * check a block
 */
in3_ret_t btc_verify_blockcount(btc_target_conf_t* conf, in3_vctx_t* vc) {
  UNUSED_VAR(conf);
  // verify the blockheader
  // TODO verify the proof
  return vc->proof ? IN3_OK : vc_err(vc, "missing the proof");
}

/**
 * check a block
 */
in3_ret_t btc_verify_block(btc_target_conf_t* conf, in3_vctx_t* vc, bytes32_t block_hash, int verbose, bool full_block) {
  uint8_t   block_header[80];
  bytes32_t block_target, hash, tmp, tmp2;
  in3_ret_t ret              = IN3_OK;
  uint32_t  block_number     = 0;
  bytes_t   finality_headers = d_bytes(d_get(vc->proof, key("final")));
  if (!vc->proof) return vc_err(vc, "missing the proof");
  if (verbose)
    btc_serialize_block_header(vc->result, block_header);      // we need to serialize the header first, so we can check the hash
  else                                                         //
    hex_to_bytes(d_string(vc->result), 160, block_header, 80); // or use the first 80 byte of the block

  // verify the blockheader
  if ((ret = btc_verify_header(vc, block_header, hash, block_target, &block_number, NULL, vc->proof))) return ret;
  if ((ret = btc_check_finality(vc, hash, vc->client->finality, finality_headers, block_target, block_number))) return ret;
  if ((ret = btc_check_target(conf, vc, block_number, block_target, finality_headers, bytes(block_header, 80)))) return ret;

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
        hex_to_bytes(verbose == 1 ? d_string(iter.token) : d_get_string(iter.token, key("txid")), 64, tx_hashes[i], 32);              // and copy the hash into the array
      btc_merkle_create_root(tx_hashes, tx_count, tmp);                                                                               // calculate the merkle root
      _free(tx_hashes);                                                                                                               // cleanup
      rev_copy(tmp2, tmp);                                                                                                            // we need to turn it into little endian be cause ini the header it is store as le.
      if (memcmp(tmp2, btc_block_get(bytes(block_header, 80), BTC_B_MERKLE_ROOT).data, 32)) return vc_err(vc, "Invalid Merkle root"); // compare the hash
                                                                                                                                      //
      btc_target_from_block(bytes(block_header, 80), tmp2);                                                                           // get current target
      uint64_t difficulty = 0xFFFF000000000000L / bytes_to_long(tmp2 + 4, 8);                                                         // and calc the difficulty
      if (difficulty >> 2 != d_get_long(vc->result, key("difficulty")) >> 2) return vc_err(vc, "Wrong difficulty");                   // which must match the one in the json
      if (!equals_hex(bytes(block_hash, 32), d_get_string(vc->result, K_HASH))) return vc_err(vc, "Wrong blockhash in json");         // check the requested hash
      if (d_get_int(vc->result, key("nTx")) != (int32_t) tx_count) return vc_err(vc, "Wrong nTx");                                    // check the nuumber of transactions
    }
    else {
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
      if (!equals_hex(bytes(hash, 32), d_get_string(vc->result, key("nextblockhash"))))
        return vc_err(vc, "Invalid nextblockhash");
    }
    int32_t v = (block_header[3] << 24) | (block_header[2] << 16) | (block_header[1] << 8) | block_header[0];
    if (v != d_get_int(vc->result, key("version"))) return vc_err(vc, "Invalid version");
  }

  return IN3_OK;
}

in3_ret_t btc_verify_target_proof(btc_target_conf_t* conf, in3_vctx_t* vc, d_token_t* params) {
  if (d_len(params) != 5) return vc_err(vc, "must have 5 params!");
  bytes32_t hash, block_target;
  in3_ret_t ret          = IN3_OK;
  uint32_t  block_number = 0;
  if (conf->max_diff != (uint_fast16_t) d_get_int_at(params, 2)) return vc_err(vc, "invalid max_diff");
  if (conf->max_daps != (uint_fast16_t) d_get_int_at(params, 3)) return vc_err(vc, "invalid max_daps");
  //  if (conf->dap_limit != (uint_fast16_t) d_get_int_at(params, 4)) return vc_err(vc, "invalid dap_limit");

  for (d_iterator_t iter = d_iter(vc->result); iter.left; d_iter_next(&iter)) {
    if (d_type(iter.token) != T_OBJECT) return vc_err(vc, "invalid type for proof");
    bytes_t header           = d_bytes(d_get(iter.token, K_BLOCK));
    bytes_t finality_headers = d_bytes(d_get(iter.token, key("final")));
    if (header.len != 80) return vc_err(vc, "invalid header");

    if ((ret = btc_verify_header(vc, header.data, hash, block_target, &block_number, NULL, iter.token))) return ret;
    if ((ret = btc_check_finality(vc, hash, vc->client->finality, finality_headers, block_target, block_number))) return ret;
    if ((ret = btc_check_target(conf, vc, block_number, block_target, finality_headers, header))) return ret;
  }

  return IN3_OK;
}

static in3_ret_t in3_verify_btc(btc_target_conf_t* conf, in3_vctx_t* vc) {
  // we only verify BTC
  if (vc->chain->type != CHAIN_BTC) return IN3_EIGNORE;

  // make sure we want to verify
  if (in3_req_get_proof(vc->req, vc->index) == PROOF_NONE) return IN3_OK;

  // do we have a result? if not it is a vaslid error-response
  if (!vc->result || d_type(vc->result) == T_NULL) return IN3_OK;

  // make sure the conf is filled with data from the cache
  btc_check_conf(vc->req, conf);
  d_token_t* params = d_get(vc->request, K_PARAMS);
  bytes32_t  hash;

#if !defined(RPC_ONLY) || defined(RPC_GETBLOCK)
  if (VERIFY_RPC("getblock")) {
    // mark zksync as experimental
    REQUIRE_EXPERIMENTAL(vc->req, "btc")

    d_token_t* block_hash = d_get_at(params, 0);
    if (d_len(params) < 1 || d_type(params) != T_ARRAY || d_type(block_hash) != T_STRING || d_len(block_hash) != 64) return vc_err(vc, "Invalid params");
    hex_to_bytes(d_string(block_hash), 64, hash, 32);
    return btc_verify_block(conf, vc, hash, d_len(params) > 1 ? d_get_int_at(params, 1) : 1, true);
  }
#endif
#if !defined(RPC_ONLY) || defined(RPC_GETBLOCKCOUNT)
  if (VERIFY_RPC("getblockcount")) {
    REQUIRE_EXPERIMENTAL(vc->req, "btc")
    return btc_verify_blockcount(conf, vc);
  }
#endif
#if !defined(RPC_ONLY) || defined(RPC_GETBLOCKHEADER)
  if (VERIFY_RPC("getblockheader")) {
    REQUIRE_EXPERIMENTAL(vc->req, "btc")
    d_token_t* block_hash = d_get_at(params, 0);
    if (d_len(params) < 1 || d_type(params) != T_ARRAY || d_type(block_hash) != T_STRING || d_len(block_hash) != 64) return vc_err(vc, "Invalid blockhash");
    hex_to_bytes(d_string(block_hash), 64, hash, 32);
    return btc_verify_block(conf, vc, hash, d_len(params) > 1 ? d_get_int_at(params, 1) : 1, false);
  }
#endif
#if !defined(RPC_ONLY) || defined(RPC_BTC_PROOFTARGET)
  if (VERIFY_RPC("btc_proofTarget")) {
    REQUIRE_EXPERIMENTAL(vc->req, "btc")
    return btc_verify_target_proof(conf, vc, params);
  }
#endif
#if !defined(RPC_ONLY) || defined(RPC_GETRAWTRANSACTION)

  if (VERIFY_RPC("getrawtransaction")) {
    REQUIRE_EXPERIMENTAL(vc->req, "btc")
    d_token_t* tx_id      = d_get_at(params, 0);
    bool       json       = d_len(params) < 2 ? d_type(vc->result) == T_OBJECT : d_get_int_at(params, 1);
    d_token_t* block_hash = d_get_at(params, 2);
    if (!tx_id || d_type(tx_id) != T_STRING || d_len(tx_id) != 64) return vc_err(vc, "Invalid tx_id");
    bytes32_t tx_hash_bytes;
    hex_to_bytes(d_string(tx_id), 64, tx_hash_bytes, 32);
    if (block_hash) hex_to_bytes(d_string(block_hash), 64, hash, 32);
    return btc_verify_tx(conf, vc, tx_hash_bytes, json, block_hash ? hash : NULL);
  }
#endif
  // TODO: Uncomment and implement functions bellow once conditions are met
  // #if !defined(RPC_ONLY) || defined(RPC_GETUTXOS)

  //   // fetch whole list of utxos
  //   if (VERIFY_RPC("getutxos")) {
  //     REQUIRE_EXPERIMENTAL(vc->req, "btc")
  //     // Get raw unsigned transaction
  //     // As we will have custody of the user priv keys, this should be obtained from our server somehow
  //     // sign transaction
  //     // return raw signed transaction
  //   }
  // #endif

  // #if !defined(RPC_ONLY) || defined(RPC_SELECTUTXOS)

  //   // TODO: Define if it is really necessary to have an RPC for this or if we should
  //   // only do this in the background when sending a transaction.
  //   if (VERIFY_RPC("selectutxos")) {
  //     REQUIRE_EXPERIMENTAL(vc->req, "btc")
  //     // Get list of outputs we want to send in out transaction
  //     // Get list of unspended transaction outputs (utxos)
  //     // Select "best" group of utxos to meet the requirements of our
  //     // return a list with the selected utxos
  //   }
  // #endif

  // #if !defined(RPC_ONLY) || defined(RPC_SIGNTRANSACTION)

  //   if (VERIFY_RPC("signtransaction")) {
  //     REQUIRE_EXPERIMENTAL(vc->req, "btc")
  //     // Get raw unsigned transaction
  //     // As we will have custody of the user priv keys, this should be obtained from our server somehow
  //     // sign transaction
  //     // return raw signed transaction
  //   }
  // #endif

  // #if !defined(RPC_ONLY) || defined(RPC_SENDRAWTRANSACTION)

  //   if (VERIFY_RPC("sendrawtransaction")) {
  //     REQUIRE_EXPERIMENTAL(vc->req, "btc")
  //     // Get raw signed transaction
  //     // verify if transaction is well-formed and signed before sending
  //     // send transaction to in3 server
  //     // return success or error code
  //   }
  // #endif
  return IN3_EIGNORE;
}

in3_ret_t btc_get_addresses(btc_target_conf_t* conf, in3_rpc_handle_ctx_t* ctx) {
  UNUSED_VAR(conf);
  in3_req_t* sub = req_find_required(ctx->req, "getrawtransaction", NULL);
  if (sub) { // do we have a result?
    switch (in3_req_state(sub)) {
      case REQ_ERROR:
        return req_set_error(ctx->req, sub->error, sub->verification_state ? sub->verification_state : IN3_ERPC);
      case REQ_SUCCESS: {
        d_token_t* result = d_get(sub->responses[0], K_RESULT);
        if (result) {
          sb_add_json(in3_rpc_handle_start(ctx), "", result);
        }
        else {
          char* error_msg = d_get_string(d_get(sub->responses[0], K_ERROR), K_MESSAGE);
          return req_set_error(ctx->req, error_msg ? error_msg : "Unable to get addresses", IN3_ERPC);
        }
        return in3_rpc_handle_finish(ctx);
      }
      case REQ_WAITING_TO_SEND:
      case REQ_WAITING_FOR_RESPONSE:
        return IN3_WAITING;
    }
  }

  char* txid;
  char* blockhash;
  TRY_PARAM_GET_REQUIRED_STRING(txid, ctx, 0);
  TRY_PARAM_GET_STRING(blockhash, ctx, 1, NULL);

  sb_t sb = {0};
  sb_add_chars(&sb, "\"");
  sb_add_chars(&sb, txid);
  sb_add_chars(&sb, "\",0");
  if (blockhash) {
    sb_add_chars(&sb, ",\"");
    sb_add_chars(&sb, blockhash);
    sb_add_chars(&sb, "\"");
  }

  d_token_t* result = NULL;
  TRY_FINAL(req_send_sub_request(ctx->req, "getrawtransaction", sb.data, NULL, &result, NULL), _free(sb.data));

  bytes_t transaction = d_get_bytes(result, key("result"));

  // Create transaction context
  btc_tx_ctx_t tx_ctx;
  btc_init_tx_ctx(&tx_ctx);
  btc_parse_tx(transaction, &tx_ctx.tx);

  // Extract all outputs
  uint8_t* p   = tx_ctx.tx.output.data;
  uint8_t* end = p + tx_ctx.tx.output.len;
  while (p < end) {
    btc_tx_out_t new_output;
    p = btc_parse_tx_out(p, &new_output);
    btc_add_output_to_tx(ctx->req, &tx_ctx, &new_output);
  }

  // Build return object
  sb_t addrs = {0};
  sb_add_chars(&addrs, "[");

  // -- For each output, extract addresses or public keys
  for (uint32_t i = 0; i < tx_ctx.output_count; i++) {
    sb_add_chars(&addrs, "{\"index\":");
    sb_add_int(&addrs, i);

    tx_ctx.outputs[i].script.type = btc_get_script_type(&tx_ctx.outputs[i].script.data);
    sb_add_chars(&addrs, ",\"script_type\":\"");
    sb_add_chars(&addrs, btc_script_type_to_string(tx_ctx.outputs[i].script.type));
    sb_add_chars(&addrs, "\",\"addrs\":[");

    if (tx_ctx.outputs[i].script.type == BTC_P2MS) {
      // extract public keys
      bytes_t* pub_key_list = NULL;
      uint32_t pub_key_list_len;
      pub_key_list_len = extract_public_keys_from_multisig(tx_ctx.outputs[i].script.data, pub_key_list);
      if (!pub_key_list || pub_key_list_len == 0) return req_set_error(ctx->req, "Error while parsing p2ms public keys", IN3_ERPC);

      // Add public keys do return
      for (uint32_t j = 0; j < pub_key_list_len; j++) {
        if (j > 0) sb_add_chars(&addrs, ",");
        sb_add_bytes(&addrs, "\"", &pub_key_list[j], pub_key_list[j].len, false);
        sb_add_chars(&addrs, "\"");
      }
    }
    else if (script_is_standard(tx_ctx.outputs[i].script.type)) {
      // extract btc address
      btc_address_t addr;
      extract_address_from_output(&tx_ctx.outputs[i], &addr);
      // TODO: Add base58 or bech32 encoding instead of bytes
      bytes_t addr_as_bytes = bytes(addr.as_bytes, BTC_ADDRESS_SIZE_BYTES);
      sb_add_bytes(&addrs, "\"", &addr_as_bytes, addr_as_bytes.len, false);
      sb_add_chars(&addrs, "\"");
    }
    sb_add_chars(&addrs, "]}");
  }
  sb_add_chars(&addrs, "]");

  TRY_FINAL(in3_rpc_handle_with_string(ctx, addrs.data), _free(addrs.data));
  return in3_rpc_handle_finish(ctx);
}

in3_ret_t send_transaction(btc_target_conf_t* conf, in3_rpc_handle_ctx_t* ctx) {
  UNUSED_VAR(conf);
  // This is the RPC that abstracts most of what is done in the background before sending a transaction:

  in3_req_t* sub = req_find_required(ctx->req, "sendrawtransaction", NULL);
  if (sub) { // do we have a result?
    switch (in3_req_state(sub)) {
      case REQ_ERROR:
        return req_set_error(ctx->req, sub->error, sub->verification_state ? sub->verification_state : IN3_ERPC);
      case REQ_SUCCESS: {
        d_token_t* result = d_get(sub->responses[0], K_RESULT);
        if (result) {
          sb_add_json(in3_rpc_handle_start(ctx), "", result);
        }
        else {
          char* error_msg = d_get_string(d_get(sub->responses[0], K_ERROR), K_MESSAGE);
          return req_set_error(ctx->req, error_msg ? error_msg : "Unable to send transaction", IN3_ERPC);
        }
        return in3_rpc_handle_finish(ctx);
      }
      case REQ_WAITING_TO_SEND:
      case REQ_WAITING_FOR_RESPONSE:
        return IN3_WAITING;
    }
  }

  in3_req_t* req    = ctx->req;
  d_token_t* params = ctx->params;

  // btc "send transaction" data
  btc_account_pub_key_t default_account;
  btc_tx_ctx_t          tx_ctx;
  btc_init_tx_ctx(&tx_ctx);

  // first parameter is the btc address which shall receive the remaining change, discounting fees, after transaction is complete
  // TODO: Receive address as Base58 or BECH32 encoding, instead of bytes
  bytes_t from_addr = d_bytes(d_get_at(params, 0));
  if (from_addr.len != BTC_ADDRESS_SIZE_BYTES) return req_set_error(req, "ERROR: Invalid btc address", IN3_EINVAL);

  // second parameter is the ethereum account used by the signer api
  d_token_t* sig_acc = d_get_at(params, 1);
  if (!sig_acc || d_type(sig_acc) != T_OBJECT) return req_set_error(req, "ERROR: Invalid signing account", IN3_EINVAL);
  default_account.account = d_bytes(d_get(sig_acc, K_ADDRESS));
  default_account.pub_key = d_bytes(d_get(sig_acc, key("btc_pub_key")));
  if (!default_account.account.data || !default_account.pub_key.data) return req_set_error(req, "ERROR: Required signing account data is null or missing", IN3_EINVAL);
  if (!btc_public_key_is_valid((const bytes_t*) &default_account.pub_key)) return req_set_error(req, "ERROR: Provided btc public key has invalid data format", IN3_EINVAL);

  // third parameter are the transaction outputs data
  d_token_t* output_data = d_get_at(params, 2);
  if (!output_data || d_type(output_data) != T_ARRAY || d_len(output_data) < 1) return req_set_error(req, "ERROR: Invalid transaction output data", IN3_EINVAL);
  btc_prepare_outputs(req, &tx_ctx, output_data);

  // forth parameter is utxo data
  d_token_t* utxo_data = d_get_at(params, 3);
  if (!utxo_data || d_type(utxo_data) != T_ARRAY || d_len(utxo_data) < 1) return req_set_error(req, "ERROR: Invalid unspent outputs (utxos) data", IN3_EINVAL);
  btc_prepare_utxos(req, &tx_ctx, &default_account, utxo_data);

  // create output for receiving the transaction "change", discounting miner fee
  uint32_t     miner_fee = 0, outputs_total = 0, utxo_total = 0;
  btc_tx_out_t tx_out_change;
  btc_init_tx_out(&tx_out_change);
  tx_out_change.value       = utxo_total - miner_fee - outputs_total;
  tx_out_change.script.data = btc_build_locking_script(&from_addr, BTC_P2PKH, NULL, 0);
  tx_out_change.script.type = btc_get_script_type(&tx_out_change.script.data);
  btc_add_output_to_tx(req, &tx_ctx, &tx_out_change);

  // Verify segwit
  btc_set_segwit(&tx_ctx);

  // Try signing the transaction
  TRY(btc_sign_tx(ctx->req, &tx_ctx));

  bytes_t* signed_tx = b_new(NULL, btc_get_raw_tx_size(&tx_ctx.tx));
  btc_serialize_tx(req, &tx_ctx.tx, signed_tx);
  sb_t sb = {0};
  sb_add_rawbytes(&sb, "\"", *signed_tx, 0);
  sb_add_chars(&sb, "\"");

  // Now that we wrote the request, we can free all allocated memory
  b_free(signed_tx);
  btc_free_tx_ctx(&tx_ctx);

  // finally, send transaction
  d_token_t* result = NULL;
  TRY_FINAL(req_send_sub_request(req, "sendrawtransaction", sb.data, NULL, &result, NULL), _free(sb.data));
  return in3_rpc_handle_finish(ctx);
}

static in3_ret_t btc_handle_intern(btc_target_conf_t* conf, in3_rpc_handle_ctx_t* ctx) {
  in3_req_t* req = ctx->req;
  // we only handle BTC
  if (req->client->chain.type != CHAIN_BTC) return IN3_EIGNORE;

  // make sure the conf is filled with data from the cache
  btc_check_conf(req, conf);

#if !defined(RPC_ONLY) || defined(RPC_GETADDRESSES)
  TRY_RPC("getaddresses", btc_get_addresses(conf, ctx))
#endif
#if !defined(RPC_ONLY) || defined(RPC_SENDTRANSACTION)

  // SERVER: sendtransaction(raw_signed_tx)
  TRY_RPC("sendtransaction", send_transaction(conf, ctx))
#endif
  return IN3_EIGNORE;
}

static in3_ret_t handle_btc(void* pdata, in3_plugin_act_t action, void* pctx) {
  btc_target_conf_t* conf = pdata;
  switch (action) {
    case PLGN_ACT_TERM: {
      if (conf->data.data) _free(conf->data.data);
      _free(conf);
      return IN3_OK;
    }
    case PLGN_ACT_CONFIG_GET: {
      in3_get_config_ctx_t* cctx = pctx;
      sb_add_chars(cctx->sb, ",\"maxDAP\":");
      sb_add_int(cctx->sb, conf->max_daps);
      sb_add_chars(cctx->sb, ",\"maxDiff\":");
      sb_add_int(cctx->sb, conf->max_diff);
      return IN3_OK;
    }
    case PLGN_ACT_CONFIG_SET: {
      in3_configure_ctx_t* cctx = pctx;
      if (d_is_key(cctx->token, CONFIG_KEY("maxDAP")))
        conf->max_daps = d_int(cctx->token);
      else if (d_is_key(cctx->token, CONFIG_KEY("maxDiff")))
        conf->max_diff = d_int(cctx->token);
      else
        return IN3_EIGNORE;
      return IN3_OK;
    }
    case PLGN_ACT_RPC_HANDLE:
      return btc_handle_intern(conf, pctx);
    case PLGN_ACT_RPC_VERIFY:
      return in3_verify_btc(conf, pctx);
    default:
      return IN3_ENOTSUP;
  }
}

in3_ret_t in3_register_btc(in3_t* c) {
  in3_register_eth_nano(c);
  // init the config
  btc_target_conf_t* tc = _calloc(1, sizeof(btc_target_conf_t));
  tc->max_daps          = 20;
  tc->max_diff          = 10;
  tc->dap_limit         = 20;
  return in3_plugin_register(c, PLGN_ACT_RPC_VERIFY | PLGN_ACT_TERM | PLGN_ACT_CONFIG_GET | PLGN_ACT_CONFIG_SET | PLGN_ACT_RPC_HANDLE, handle_btc, tc, false);
}