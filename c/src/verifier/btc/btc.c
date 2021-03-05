#include "btc.h"
#include "../../core/client/keys.h"
#include "../../core/client/plugin.h"
#include "../../core/client/request_internal.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "../../verifier/eth1/nano/eth_nano.h"
#include "btc_merkle.h"
#include "btc_serialize.h"
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
  bytes_t     merkle_proof = d_to_bytes(d_get(proof, key("cbtxMerkleProof")));
  bytes_t     tx           = d_to_bytes(d_get(proof, key("cbtx")));
  bytes32_t   tx_id;
  btc_tx_t    tx_data;
  btc_tx_in_t tx_in;

  if (*header.data == 1 && memiszero(header.data + 1, 3)) {
    *dst_block_number = (uint32_t) d_get_int(proof, key("height"));
    if (!*dst_block_number) return vc_err(vc, "missing height in proof for blocks pre bip34");
#ifdef BTC_PRE_BPI34
    return check_pre_bip34(vc, d_to_bytes(d_get(proof, key("final"))), *dst_block_number);
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
        if (!hex || !equals_hex(tx_in.script, hex)) return vc_err(vc, "invalid coinbase");
      }
      else {
        // txid
        hex = d_get_string(iter.token, key("txid"));
        if (!equals_hex_rev(bytes(tx_in.prev_tx_hash, 32), hex)) return vc_err(vc, "invalid vin.txid");

        // vout
        if (d_get_int(iter.token, key("vout")) != (int32_t) tx_in.prev_tx_index) return vc_err(vc, "invalid vin.vout");

        // sig.hex
        hex = d_get_string(d_get(iter.token, key("scriptSig")), key("hex"));
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
      if (d_get_int(iter.token, key("n")) != n) return vc_err(vc, "invalid vout.n");

      // sig.hex
      char* hex = d_get_string(d_get(iter.token, key("scriptPubKey")), key("hex"));
      if (!equals_hex(tx_out.script, hex)) return vc_err(vc, "invalid vout.hex");
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
  if ((ret = btc_check_target(conf, vc, block_number, block_target, finality_headers, header))) return ret;

  return IN3_OK;
}

/**
 * check a block
 */
in3_ret_t btc_verify_blockcount(btc_target_conf_t* conf, in3_vctx_t* vc) {
  UNUSED_VAR(conf);
  // verify the blockheader
  //TODO verify the proof
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
  bytes_t   finality_headers = d_to_bytes(d_get(vc->proof, key("final")));
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
    bytes_t header           = d_to_bytes(d_get(iter.token, K_BLOCK));
    bytes_t finality_headers = d_to_bytes(d_get(iter.token, key("final")));
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
  btc_check_conf(vc->client, conf);
  d_token_t* params = d_get(vc->request, K_PARAMS);
  bytes32_t  hash;

  if (strcmp(vc->method, "getblock") == 0) {
    // mark zksync as experimental
    REQUIRE_EXPERIMENTAL(vc->req, "btc")

    d_token_t* block_hash = d_get_at(params, 0);
    if (d_len(params) < 1 || d_type(params) != T_ARRAY || d_type(block_hash) != T_STRING || d_len(block_hash) != 64) return vc_err(vc, "Invalid params");
    hex_to_bytes(d_string(block_hash), 64, hash, 32);
    return btc_verify_block(conf, vc, hash, d_len(params) > 1 ? d_get_int_at(params, 1) : 1, true);
  }
  if (strcmp(vc->method, "getblockcount") == 0) {
    REQUIRE_EXPERIMENTAL(vc->req, "btc")
    return btc_verify_blockcount(conf, vc);
  }
  if (strcmp(vc->method, "getblockheader") == 0) {
    REQUIRE_EXPERIMENTAL(vc->req, "btc")
    d_token_t* block_hash = d_get_at(params, 0);
    if (d_len(params) < 1 || d_type(params) != T_ARRAY || d_type(block_hash) != T_STRING || d_len(block_hash) != 64) return vc_err(vc, "Invalid blockhash");
    hex_to_bytes(d_string(block_hash), 64, hash, 32);
    return btc_verify_block(conf, vc, hash, d_len(params) > 1 ? d_get_int_at(params, 1) : 1, false);
  }
  if (strcmp(vc->method, "btc_proofTarget") == 0) {
    REQUIRE_EXPERIMENTAL(vc->req, "btc")
    return btc_verify_target_proof(conf, vc, params);
  }
  if (strcmp(vc->method, "getrawtransaction") == 0) {
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
      if (cctx->token->key == key("maxDAP"))
        conf->max_daps = d_int(cctx->token);
      else if (cctx->token->key == key("maxDiff"))
        conf->max_diff = d_int(cctx->token);
      else
        return IN3_EIGNORE;
      return IN3_OK;
    }
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
  return in3_plugin_register(c, PLGN_ACT_RPC_VERIFY | PLGN_ACT_TERM | PLGN_ACT_CONFIG_GET | PLGN_ACT_CONFIG_SET, handle_btc, tc, false);
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

*/