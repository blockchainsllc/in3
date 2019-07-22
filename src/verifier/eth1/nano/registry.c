#include "../../../core/client/context.h"
#include "../../../core/client/keys.h"
#include "../../../core/util/mem.h"
#include "../../../verifier/eth1/nano/eth_nano.h"
#include "../../../verifier/eth1/nano/merkle.h"
#include "../../../verifier/eth1/nano/rlp.h"
#include "../../../verifier/eth1/nano/serialize.h"
#include <string.h>

#define SERVER_STRUCT_SIZE 6

static void big_add(bytes32_t a, uint8_t* b, wlen_t len_b) {
  optimize_len(b, len_b);
  uint8_t *     pa = a + 31, *pb = b + len_b - 1;
  uint_fast16_t carry = 0;
  do {
    carry += *pa + *pb;
    *pa = carry & 0xFF;
    carry >>= 8;
    pb--, pa--;
  } while (b == pb);

  while (carry && pa >= a) {
    carry += *pa;
    *pa = carry & 0xFF;
    carry >>= 8;
    pa--;
  }
}

static in3_ret_t get_storage_value(d_token_t* storage_proofs, uint8_t* skey, bytes32_t value) {
  uint_fast16_t key_len = 32;
  optimize_len(skey, key_len);
  bytes_t  tmp;
  uint8_t *p, l;
  for (d_iterator_t it = d_iter(storage_proofs); it.left; d_iter_next(&it)) {
    tmp = d_to_bytes(d_get(it.token, K_KEY));
    p   = tmp.data;
    l   = tmp.len;
    optimize_len(p, l);
    if ((l == 0 && key_len == 1 && !*skey) || (l == key_len && !memcmp(p, skey, l))) {
      tmp = d_to_bytes(d_get(it.token, K_VALUE));
      if (tmp.len < 32) {
        memset(value, 0, 32 - tmp.len);
        memcpy(value + 32 - tmp.len, tmp.data, tmp.len);
      } else
        memcpy(value, tmp.data, 32);
      return IN3_OK;
    }
  }
  return IN3_EFIND;
}

static in3_ret_t check_storage(in3_vctx_t* vc, d_token_t* storage_proofs, bytes32_t skey, bytes32_t value) {
  bytes32_t tmp;
  if (get_storage_value(storage_proofs, skey, tmp) != IN3_OK) return vc_err(vc, "Could not find the storage key");
  return memcmp(value, tmp, 32) ? vc_err(vc, "wrong storage value") : IN3_OK;
}

static uint8_t* as_bytes32(bytes32_t dst, bytes_t b) {
  if (b.len >= 32)
    memcpy(dst, b.data, 32);
  else {
    memset(dst, 0, 32);
    memcpy(dst + 32 - b.len, b.data, b.len);
  }
  return dst;
}

static void create_random_indexes(const uint32_t total_servers, const uint32_t node_limit, bytes_t* src_seed, uint32_t* seed_indexes, uint32_t seed_len, uint32_t* indexes) {
  bytes32_t seed_data;
  bytes_t   seed = {.data = seed_data, .len = 32};
  memcpy(seed_data, src_seed->data, 32);
  uint32_t len  = seed_len, i;
  uint64_t step = bytes_to_long(seed_data, 6), pos = bytes_to_long(seed_data + 6, 6) % total_servers;
  if (seed_len) memcpy(indexes, seed_indexes, sizeof(uint32_t) * seed_len);
  while (len < node_limit) {
    bool exists = false;
    for (i = 0; i < len; i++) {
      if (indexes[i] == pos) {
        exists = true;
        break;
      }
    }
    if (exists) {
      sha3_to(&seed, seed_data);
      step = bytes_to_long(seed_data, 6);
    } else
      indexes[len++] = pos;
    pos = (pos + step) % total_servers;
  }
}

static uint8_t* get_storage_array_key(uint32_t pos, uint32_t array_index, uint32_t struct_size, uint32_t struct_pos, bytes32_t dst) {
  memset(dst, 0, 32);
  int_to_bytes(pos, dst + 28);
  if (!struct_size) return dst;
  bytes_t p = bytes(dst, 32);
  sha3_to(&p, dst);

  uint8_t tmp[4];
  int_to_bytes(array_index * struct_size + struct_pos, tmp);
  big_add(dst, tmp, 4);
  return dst;
}

static in3_ret_t verify_nodelist_data(in3_vctx_t* vc, const uint32_t node_limit, bytes_t* seed, d_token_t* required_addresses, d_token_t* server_list, d_token_t* storage_proofs) {
  bytes32_t skey, svalue;
  uint32_t  total_servers = d_get_intk(vc->result, K_TOTAL_SERVERS);

  TRY(check_storage(vc, storage_proofs, get_storage_array_key(0, 0, 0, 0, skey), as_bytes32(svalue, d_to_bytes(d_get(vc->result, K_TOTAL_SERVERS)))));

  if (node_limit && node_limit < total_servers) {
    if (d_len(server_list) != (int) node_limit) return vc_err(vc, "wrong length of the nodes!");
    const uint32_t seed_len = required_addresses ? d_len(required_addresses) : 0;
    uint32_t       seed_indexes[seed_len], i = 0;

    // check if the required addresses are part of the list
    // and create the index-list
    if (seed_len) {
      for (d_iterator_t itr = d_iter(required_addresses); itr.left; d_iter_next(&itr), i++) {
        bool     found = false;
        bytes_t* adr   = d_bytesl(itr.token, 20);
        for (d_iterator_t itn = d_iter(server_list); itn.left; d_iter_next(&itn)) {
          if (b_cmp(d_get_byteskl(itn.token, K_ADDRESS, 20), adr)) {
            found           = true;
            seed_indexes[i] = d_get_intk(itn.token, K_INDEX);
            break;
          }
        }
        if (!found) return vc_err(vc, "could not find required address");
      }
    }

    // create indexes
    uint32_t indexes[node_limit];
    create_random_indexes(total_servers, node_limit, seed, seed_indexes, seed_len, indexes);

    // check that we have the correct indexes in the nodelist
    i = 0;
    for (d_iterator_t it = d_iter(server_list); it.left; d_iter_next(&it), i++) {
      if (d_get_intk(it.token, K_INDEX) != indexes[i]) return vc_err(vc, "wrong index in partial nodelist");
    }
  } else if ((int) total_servers != d_len(server_list))
    return vc_err(vc, "wrong number of nodes in the serverlist");

  // now check the content of the nodelist
  for (d_iterator_t it = d_iter(server_list); it.left; d_iter_next(&it)) {
    uint32_t index = d_get_intk(it.token, K_INDEX);

    // check the owner
    if (!d_get(it.token, K_ADDRESS)) return vc_err(vc, "no owner in nodelist");
    memset(svalue, 0, 32);
    long_to_bytes(d_get_longkd(it.token, K_TIMEOUT, 0), svalue + 4);
    memcpy(svalue + 12, d_get_byteskl(it.token, K_ADDRESS, 20)->data, 20);
    TRY(check_storage(vc, storage_proofs, get_storage_array_key(0, index, SERVER_STRUCT_SIZE, 1, skey), svalue));

    // check the deposit
    TRY(get_storage_value(storage_proofs, get_storage_array_key(0, index, SERVER_STRUCT_SIZE, 2, skey), svalue));
    uint64_t deposit = bytes_to_long(svalue, 32);
    if (d_get_longk(it.token, K_DEPOSIT) != deposit) return vc_err(vc, "wrong deposit");

    // check props
    TRY(check_storage(vc, storage_proofs, get_storage_array_key(0, index, SERVER_STRUCT_SIZE, 3, skey), as_bytes32(svalue, d_to_bytes(d_get(it.token, K_PROPS)))));

    // check url
    TRY(get_storage_value(storage_proofs, get_storage_array_key(0, index, SERVER_STRUCT_SIZE, 0, skey), svalue));
    const char* url = d_get_stringk(it.token, K_URL);
    if (!url) return vc_err(vc, "missing url");
    if (svalue[31] % 2) {
      // the url-value is concated from multiple values.
      uint32_t len  = (bytes_to_int(svalue + 28, 4) - 1) >> 1;
      uint8_t  inc  = 1;
      bytes_t  hash = bytes(skey, 32);
      sha3_to(&hash, skey);
      if (len != strlen(url)) return vc_err(vc, "wrong url");
      for (uint32_t n = 0; n <= len >> 5; n++, big_add(skey, &inc, 1)) {
        TRY(get_storage_value(storage_proofs, skey, svalue));
        if (memcmp(svalue, url + (n << 5), min(32, len - (n << 5)))) return vc_err(vc, "wrong url");
      }
    } else if (strlen(url) != svalue[31] >> 1 || memcmp(url, svalue, svalue[31] >> 1))
      return vc_err(vc, "wrong url");
  }

  return IN3_OK;
}

in3_ret_t eth_verify_in3_nodelist(in3_vctx_t* vc, uint32_t node_limit, bytes_t* seed, d_token_t* required_addresses) {
  uint8_t         hash[32], val[36];
  bytes_t         root, **proof, *account_raw, path = {.data = hash, .len = 32};
  d_token_t *     server_list = d_get(vc->result, K_NODES), *storage_proof, *t;
  bytes_builder_t bb          = {.bsize = 36, .b = {.data = val, .len = 0}};
  if (d_type(vc->result) != T_OBJECT || !vc->proof || !server_list) return vc_err(vc, "Invalid nodeList response!");

  // verify the header
  bytes_t* blockHeader = d_get_bytesk(vc->proof, K_BLOCK);
  if (!blockHeader) return vc_err(vc, "No Block-Proof!");
  TRY(eth_verify_blockheader(vc, blockHeader, NULL));

  // check contract
  bytes_t* registry_contract = d_get_byteskl(vc->result, K_CONTRACT, 20);
  if (!registry_contract || !b_cmp(registry_contract, vc->chain->contract)) return vc_err(vc, "No or wrong Contract!");

  // check last block
  if (rlp_decode_in_list(blockHeader, BLOCKHEADER_NUMBER, &root) != 1 || bytes_to_long(root.data, root.len) < d_get_longk(vc->result, K_LAST_BLOCK_NUMBER)) return vc_err(vc, "The signature is based on older block!");

  // check accounts
  d_token_t* accounts = d_get(vc->proof, K_ACCOUNTS);
  if (!accounts || d_len(accounts) != 1) return vc_err(vc, "Invalid accounts!");
  d_token_t* account = accounts + 1;

  // verify the account proof
  if (rlp_decode_in_list(blockHeader, BLOCKHEADER_STATE_ROOT, &root) != 1) return vc_err(vc, "no state root in the header");
  if (!b_cmp(d_get_byteskl(account, K_ADDRESS, 20), registry_contract)) return vc_err(vc, "wrong address in the account proof");

  proof = d_create_bytes_vec(d_get(account, K_ACCOUNT_PROOF));
  if (!proof) return vc_err(vc, "no merkle proof for the account");
  account_raw = serialize_account(account);
  sha3_to(registry_contract, hash);
  if (!trie_verify_proof(&root, &path, proof, account_raw)) {
    _free(proof);
    b_free(account_raw);
    return vc_err(vc, "invalid account proof");
  }
  _free(proof);
  b_free(account_raw);

  // now verify storage proofs
  if (!(storage_proof = d_get(account, K_STORAGE_PROOF))) return vc_err(vc, "no stortage-proof found!");
  if ((t = d_getl(account, K_STORAGE_HASH, 32)))
    root = *d_bytes(t);
  else
    return vc_err(vc, "no storage-hash found!");

  for (d_iterator_t it = d_iter(storage_proof); it.left; d_iter_next(&it)) {
    // prepare the key
    d_bytes_to(d_get(it.token, K_KEY), hash, 32);
    sha3_to(&path, hash);

    proof = d_create_bytes_vec(d_get(it.token, K_PROOF));
    if (!proof) return vc_err(vc, "no merkle proof for the storage");

    // rlp encode the value.
    if ((bb.b.len = d_bytes_to(d_get(it.token, K_VALUE), val, -1)))
      rlp_encode_to_item(&bb);

    // verify merkle proof
    if (!trie_verify_proof(&root, &path, proof, bb.b.len ? &bb.b : NULL)) {
      _free(proof);
      return vc_err(vc, "invalid storage proof");
    }
    _free(proof);
  }

  // now verify the nodelist
  return verify_nodelist_data(vc, node_limit, seed, required_addresses, server_list, storage_proof);
}
