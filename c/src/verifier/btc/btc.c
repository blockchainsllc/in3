#include "btc.h"
#include "../../core/client/keys.h"
#include "../../core/util/mem.h"
#include "../../core/util/utils.h"
#include "btc_merkle.h"
#include "btc_serialize.h"
#include <stdlib.h>
#include <string.h>

static inline uint32_t btc_epoch(uint32_t block_number) {
  return block_number / 2016;
}

// check if 2 byte arrays are equal where one is a bytes while the other one is a hex string (without 0x)
static bool equals_hex(bytes_t data, char* hex) {
  uint32_t sl = hex ? strlen(hex) : 0, bl = sl >> 1;                                                              // calc len of bytes from hex
  if (bl != data.len || sl % 1) return false;                                                                     // we do not support odd length of hex
  for (uint32_t i = 0; i < bl; i++) {                                                                             // compare each byte
    if (data.data[i] != ((hexchar_to_int(hex[i << 1]) << 4) | (hexchar_to_int(hex[(i << 1) + 1])))) return false; // cancel on first difference
  }
  return true;
}

// handle the request before sending it out the the node.
// not needed yet.
static in3_ret_t btc_handle_intern(in3_ctx_t* ctx, in3_response_t** response) {
  if (ctx->len > 1 || !response) return IN3_ENOTSUP; // internal handling is only possible for single requests (at least for now)
  return IN3_OK;
}

static uint8_t* skip_vin(uint8_t* p, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    p += 36;                      // skip prev tx(32) and txout(4)
    uint64_t tmp;                 //
    p += decode_var_int(p, &tmp); // script length
    p += tmp + 4;                 // script bytes + sequence ( 4)
  }
  return p;
}

static uint8_t* skip_vout(uint8_t* p, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    p += 8;                       // skip  value (8)
    uint64_t tmp;                 //
    p += decode_var_int(p, &tmp); // script length
    p += tmp;                     // script bytes
  }
  return p;
}

static inline bool is_witness(bytes_t tx) {
  return tx.data[4] == 0 && tx.data[5] == 1;
}

static in3_ret_t btc_txid(in3_vctx_t* vc, bytes_t tx, bytes32_t txid) {
  uint8_t* start = tx.data + 4 + (is_witness(tx) ? 2 : 0);
  uint64_t tmp;
  uint8_t* p = start + decode_var_int(start, &tmp);
  p          = skip_vin(p, (uint32_t) tmp);
  p          = p + decode_var_int(p, &tmp);
  p          = skip_vout(p, (uint32_t) tmp);

  /*
* 
01000000 // version
03 // l vin
8c091a64ddbc99f81f3fd4b2fbb5bfafa68e87d9f0a90df89ac69bf9e5f6740a000000006a4730440220481f2b3a49b202e26c73ac1b7bce022e4a74aff08473228ccf362f6043639efe02201d573e65394228ae30cfdc67b0456a5fb02af4fdbf29c2771c8a5dcfee4b2c9b012103a6ab31dcae7d90085809b58fbac506631a57dc34ed942e74ef116e0800254874ffffffff44509eb1e52041a66e8e7191f77c460ae326e12e3d158a9c13c3dfd4825e9c86000000006b483045022100ae5bd019a63aed404b743c9ebcc77fbaa657e481f745e4e47abc7cd2a0c77b5402204cdfd79646e50590386e1cb7334aaf9338016d2f3657fefa1bf3a23a0e3454e7012103427d40830bb4648442f9a4d901ea42a7bbdb8af39e9596e1a91d3b34e8f3255dffffffffff8efa7372998c2e6bd363c0046b3dbc2983ef5be12a4ac908e48a1b9ad2038a010000006a47304402200bf7c5c7caec478bf6d7e9c5127c71505034302056d12848749bbe9d57664ef3022056f564fdb4da99cde5c856211c93a820f9222af0292039a8e3dd9d429c172f320121027f3061a928f1780afceb18813215febbec05bd4b186feff66fd32128520045daffffffff02a3440000000000001976a91453196749b85367db9443ef9a5aec25cf0bdceedf88ac14f90d000000000017a9148bb2b4b848d0b6336cc64ea57ae989630f447cba8700000000
* 
*/

  bytes_t tdata;
  tdata.len  = p - start + 8;
  tdata.data = alloca(tdata.len);

  memcpy(tdata.data, tx.data, 4);                              // nVersion
  memcpy(tdata.data + 4, start, p - start);                    // txins/txouts
  memcpy(tdata.data + tdata.len - 4, tx.data + tx.len - 4, 4); //lockTime

  btc_hash(tdata, txid);
  return IN3_OK;
}

static in3_ret_t btc_block_number(in3_vctx_t* vc, uint32_t* dst_block_number) {
  bytes_t   header       = d_to_bytes(d_get(vc->proof, K_BLOCK));
  bytes_t   merkle_proof = d_to_bytes(d_get(vc->proof, key("cbtxmerkleProof")));
  bytes_t   tx           = d_to_bytes(d_get(vc->proof, key("cbtx")));
  bytes32_t tx_hash;

  if (header.len != 80) return vc_err(vc, "invalid blockheader");
  if (!merkle_proof.len) return vc_err(vc, "missing merkle proof");
  if (!tx.len) return vc_err(vc, "missing coinbase tx");

  // verify merkle proof
  if (btc_txid(vc, tx, tx_hash)) return vc_err(vc, "invalid txid!");
  if (!btc_merkle_verify_proof(btc_block_get(header, BTC_B_MERKLE_ROOT).data, merkle_proof, 0, tx_hash)) return vc_err(vc, "merkleProof failed!");

  if (tx.data[6] != 1) return vc_err(vc, "vin count needs to be 1 for coinbase tx");

  uint64_t       sig_len;
  const uint8_t* sig = tx.data + 7 + 36 + decode_var_int(tx.data + 7 + 36, &sig_len);
  *dst_block_number  = ((uint32_t) sig[1]) | (((uint32_t) sig[1]) << 8) | (((uint32_t) sig[2]) << 16);

  // 01000000 // Version
  // 0001     // Flag
  // 01       // VARINT
  /*
0000000000000000000000000000000000000000000000000000000000000000 // pre tx
ffffffff // txout index
5f // VARINT length
033e890904e2888d5e2f706f6f6c696e2e636f6d2ffabe6d6d96439d07cae2a0d0d7b459d69d6e8fbe84a28f9be160edb3c33279ebfbc7af8d010000000000000066e1bb5b9e64b5779c4872da7ee8ba921008d1689900a204000000000000ffffffff0491e3894a0000000017a91454705dd010ab50c03543a543cda327a60d9bf7af870000000000000000266a24b9e11b6ddd3fec243f225bbee268c09f1a616a2c6e5196a1e8977d59b32daf8ae52767570000000000000000266a24aa21a9edb1979b2b4464d1ef79257852017413a72a9cd2fa9048fa86052d1121f59d536c00000000000000002b6a2952534b424c4f434b3aa7a3b7405613557aba7780a86227466b8fa451f82f0cdce821aafe2400222c9901200000000000000000000000000000000000000000000000000000000000000000539da2fc

*/

  return IN3_OK;
}

/**
 * verify proof of work of the blockheader
 */
static in3_ret_t
btc_verify_header(in3_vctx_t* vc, uint8_t* block_header, bytes32_t dst_hash, bytes32_t dst_target, uint32_t* block_number, bytes32_t expected_target) {
  in3_ret_t ret = IN3_OK;
  btc_target(bytes(block_header, 80), dst_target); // check target
  btc_hash(bytes(block_header, 80), dst_hash);     // check blockhash
  if (memcmp(dst_target, dst_hash, 32) < 0) return vc_err(vc, "Invalid proof of work. the hash is greater than the target");
  if (expected_target)
    return memcmp(dst_target, expected_target, 32) == 0 ? IN3_OK : vc_err(vc, "Invalid target");

  if ((ret = btc_block_number(vc, block_number))) return ret;

  return ret;
}

static in3_ret_t btc_new_target_check(in3_vctx_t* vc, bytes32_t old_target, bytes32_t new_target) {
  bytes32_t tmp;
  memcpy(tmp, old_target, 32);
  for (int i = 0; i < 31; i++) tmp[i] = tmp[i] << 2 || tmp[i + 1] >> 6; // multiply by 4
  if (memcmp(tmp, new_target, 32) < 0) return vc_err(vc, "new target is more than 4 times the old target");
  memcpy(tmp, old_target, 32);
  for (int i = 1; i < 32; i++) tmp[i] = tmp[i] >> 2 || tmp[i - 1] << 6; // divide by 4
  if (memcmp(tmp, new_target, 32) > 0) return vc_err(vc, "new target is less than one 4th of the old target");
  return IN3_OK;
}

/**
 * verify the finality block.
 */
in3_ret_t btc_check_finality(in3_vctx_t* vc, bytes32_t block_hash, int finality, bytes_t final_blocks, bytes32_t expected_target, uint64_t block_nr) {
  if (!finality) return IN3_OK;
  bytes32_t parent_hash, tmp, target;
  memcpy(target, expected_target, 32);
  in3_ret_t ret = IN3_OK;
  memcpy(parent_hash, block_hash, 32);                                                                    // we start with the current block hash as parent
  block_nr++;                                                                                             // we start with the next block_nr
  for (int i = 0, p = 0; i < finality; i++, p += 80, block_nr++) {                                        // go through all requested finality blocks
    if ((block_nr) % 2016 == 0) {                                                                         // if we reached a epcoch limit
      i = 0;                                                                                              // we need all finality-headers again startting with the DAP-break.
      btc_target(bytes(final_blocks.data + p, 80), tmp);                                                  // read the new target from the new blockheader
      if ((ret = btc_new_target_check(vc, target, tmp))) return ret;                                      // check if the new target is within the allowed range (*/4)
      memcpy(target, tmp, 32);                                                                            // now we use the new target
    }                                                                                                     //
    if (p + 80 > (int) final_blocks.len) return vc_err(vc, "Not enough finality blockheaders");           //  the bytes need to be long enough
    rev_copy(tmp, btc_block_get(bytes(final_blocks.data + p, 80), BTC_B_PARENT_HASH).data);               // copy the parent hash of the block inito tmp
    if (memcmp(tmp, parent_hash, 32)) return vc_err(vc, "wrong parent_hash in finality block");           // check parent hash
    if ((ret = btc_verify_header(vc, final_blocks.data + p, parent_hash, tmp, NULL, target))) return ret; // check the headers proof of work and set the new parent hash
  }
  return ret;
}

in3_ret_t btc_verify_tx(in3_vctx_t* vc, uint8_t* tx_hash, bool json, uint8_t* block_hash) {
  bytes_t    data, merkle_data, header;
  bytes32_t  hash, expected_block_hash, block_target, hash2;
  d_token_t* t;
  bool       in_active_chain = true;

  if (block_hash) memcpy(expected_block_hash, block_hash, 32);

  if (json) {

    //TODO add witness support, since the txid will not be the same as the hash for a witness transaction

    // check txid
    t = d_get(vc->result, key("txid"));
    if (!t || d_type(t) != T_STRING || d_len(t) != 64) return vc_err(vc, "missing or invalid txid");
    hex_to_bytes(d_string(t), 64, hash, 32);
    if (memcmp(hash, tx_hash, 32)) return vc_err(vc, "wrong txid");

    // check hex
    t = d_get(vc->result, key("hex"));
    if (!t || d_type(t) != T_STRING) return vc_err(vc, "missing hex");
    data.len  = (d_len(t) + 1) >> 1;
    data.data = alloca(data.len);
    hex_to_bytes(d_string(t), d_len(t), data.data, data.len);

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

    // check size
    if (d_get_intk(vc->result, key("size")) != (int32_t) data.len) return vc_err(vc, "invalid size");

    // TODO check the vin and vout
    // https://bitcoin.org/en/developer-reference#getrawtransaction

  } else {

    // here we expect the raw serialized transaction
    if (!vc->result || d_type(vc->result) != T_STRING) return vc_err(vc, "expected hex-data as result");
    data.len  = (d_len(vc->result) + 1) >> 1;
    data.data = alloca(data.len);
    hex_to_bytes(d_string(vc->result), d_len(vc->result), data.data, data.len);
  }

  // hash and check transaction hash
  if (btc_txid(vc, data, hash)) return vc_err(vc, "invalid txdata");
  if (memcmp(hash, tx_hash, 32)) return vc_err(vc, "invalid hex");

  // get the header
  t = d_get(vc->proof, K_BLOCK);
  if (!t || d_type(t) != T_BYTES || d_len(t) != 80) return vc_err(vc, "missing or invalid blockheader!");
  header = d_to_bytes(t);

  // now check the merkle proof
  t = d_get(vc->proof, K_MERKLE_PROOF);
  if (!t || d_type(t) != T_BYTES) return vc_err(vc, "missing merkle proof!");
  merkle_data = d_to_bytes(t);

  // check the transactionIndex
  t = d_get(vc->proof, K_TX_INDEX);
  if (!t || d_type(t) != T_INTEGER) return vc_err(vc, "missing txIndex");

  // verify merkle proof
  if (!btc_merkle_verify_proof(btc_block_get(header, BTC_B_MERKLE_ROOT).data, merkle_data, d_int(t), tx_hash)) return vc_err(vc, "merkleProof failed!");

  // now verify the blockheader
  uint32_t  block_number;
  in3_ret_t valid_header = btc_verify_header(vc, header.data, hash, block_target, &block_number, NULL);
  if (valid_header == IN3_WAITING) return valid_header;
  if (in_active_chain != (valid_header == IN3_OK)) return vc_err(vc, "active_chain check failed!");

  // make sure we have the expected blockhash
  if ((block_hash || json) && memcmp(expected_block_hash, hash, 32)) return vc_err(vc, "invalid hash of blockheader!");

  // check finality
  return in_active_chain ? btc_check_finality(vc, hash, vc->config->finality, d_to_bytes(d_get(vc->proof, key("final"))), block_target, block_number) : IN3_OK;
}

/**
 * check a block
 */
in3_ret_t btc_verify_block(in3_vctx_t* vc, bytes32_t block_hash, bool json) {
  uint8_t   block_header[80];
  bytes32_t tmp, tmp2, block_target;
  in3_ret_t ret = IN3_OK;
  uint32_t  block_number;
  if (json)
    btc_serialize_block_header(vc->result, block_header);      // we need to serialize the header first, so we can check the hash
  else                                                         //
    hex_to_bytes(d_string(vc->result), 160, block_header, 80); // or use the first 80 byte of the block

  // verify the proof of work
  if ((ret = btc_verify_header(vc, block_header, tmp, block_target, &block_number, NULL))) return ret;

  // check blockhash
  if (memcmp(tmp, block_hash, 32)) return vc_err(vc, "Invalid blockhash");

  // check merkleroot
  if (json) {
    d_token_t* tx       = d_get(vc->result, key("tx"));                                                                             // get transactions node
    int        tx_count = d_len(tx), i = 0;                                                                                         // and count its length
    bytes32_t* tx_hashes = alloca(tx_count * sizeof(bytes32_t));                                                                    // to reserve hashes-array
    for (d_iterator_t iter = d_iter(tx); iter.left; d_iter_next(&iter), i++)                                                        // iterate through all txs
      hex_to_bytes(d_string(iter.token), 64, tx_hashes[i], 32);                                                                     // and copy the hash into the array
    btc_merkle_create_root(tx_hashes, tx_count, tmp);                                                                               // calculate the merkle root
    rev_copy(tmp2, tmp);                                                                                                            // we need to turn it into little endian be cause ini the header it is store as le.
    if (memcmp(tmp2, btc_block_get(bytes(block_header, 80), BTC_B_MERKLE_ROOT).data, 32)) return vc_err(vc, "Invalid Merkle root"); // compare the hash
                                                                                                                                    //
    btc_target(bytes(block_header, 80), tmp2);                                                                                      // get current target
    uint64_t difficulty = 0xFFFF000000000000L / bytes_to_long(tmp2 + 4, 8);                                                         // and calc the difficulty
    if (difficulty >> 2 != d_get_long(vc->result, "difficulty") >> 2) return vc_err(vc, "Wrong difficulty");                        // which must match the one in the json
    if (!equals_hex(bytes(block_hash, 32), d_get_string(vc->result, "hash"))) return vc_err(vc, "Wrong blockhash in json");         // check the requested hash
    if (d_get_int(vc->result, "nTx") != (int32_t) tx_count) return vc_err(vc, "Wrong nTx");                                         // check the nuumber of transactions

  } else {
    char*    block_hex  = d_string(vc->result);
    uint8_t* block_data = alloca(strlen(block_hex) / 2);
    bytes_t  block      = bytes(block_data, strlen(block_hex) / 2);
    hex_to_bytes(block_hex, -1, block.data, block.len);
    int        tx_count     = btc_get_transaction_count(block);
    bytes_t*   transactions = alloca(tx_count);
    bytes32_t* tx_hashes    = alloca(tx_count);
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
    return btc_check_finality(vc, block_hash, vc->ctx->requests_configs->finality, d_to_bytes(d_get(vc->proof, key("final"))), block_target, block_number);

  return IN3_OK;
}

//in3_ret_t in3_verify_transaction(in3_vctx_t* vc, bytes32_t transaction_hash, bool json) {
//TODO
//}

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
    hex_to_bytes(d_string(block_hash), 64, hash, 32);
    return btc_verify_block(vc, hash, d_len(params) > 1 ? d_get_int_at(params, 1) : 1);
  }
  if (strcmp(method, "getblockheader") == 0) {
    d_token_t* block_hash = d_get_at(params, 0);
    if (d_len(params) < 1 || d_type(params) != T_ARRAY || d_type(block_hash) != T_STRING || d_len(block_hash) != 64) return vc_err(vc, "Invalid params");
    hex_to_bytes(d_string(block_hash), 64, hash, 32);
    return btc_verify_block(vc, hash, d_len(params) > 1 ? d_get_int_at(params, 1) : 1);
  }
  if (strcmp(method, "getrawtransaction") == 0) {
    d_token_t* tx_hash    = d_get_at(params, 0);
    bool       json       = d_len(params) < 2 ? true : d_get_int_at(params, 1);
    d_token_t* block_hash = d_get_at(params, 2);
    if (!tx_hash || d_type(tx_hash) != T_STRING || d_len(tx_hash) != 64) return vc_err(vc, "Invalid tx_hash");
    bytes32_t tx_hash_bytes;
    hex_to_bytes(d_string(tx_hash), 64, tx_hash_bytes, 32);
    return btc_verify_tx(vc, tx_hash_bytes, json, block_hash ? hash : NULL);
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
  hex_to_bytes(block_hex, -1, block.data, block.len);

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

  btc_target(block, hash);
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