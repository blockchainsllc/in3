/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 * 
 * 
 * COMMERCIAL LICENSE USAGE
 * 
 * Licensees holding a valid commercial license may use this file in accordance 
 * with the commercial license agreement provided with the Software or, alternatively, 
 * in accordance with the terms contained in a written agreement between you and 
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
 * information please contact slock.it at in3@slock.it.
 * 	
 * Alternatively, this file may be used under the AGPL license as follows:
 *    
 * AGPL LICENSE USAGE
 * 
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available 
 * complete source code of licensed works and modifications, which include larger 
 * works using a licensed work, under the same license. Copyright and license notices 
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

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
  memset(seed_data, 0, 32);
  memcpy(seed_data + 32 - src_seed->len, src_seed->data, src_seed->len);
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

_NOINLINE_ static void create_node_hash(d_token_t* t, bytes32_t dst) {
  bytes_t  url    = d_to_bytes(d_get(t, K_URL)), val;
  int      l      = 92 + url.len;
  uint8_t* buffer = alloca(l);
  memset(buffer, 0, l);

  bytes_t data = bytes(buffer, l);
  if ((val = d_to_bytes(d_get(t, K_DEPOSIT))).data && val.len < 33) memcpy(buffer + 32 - val.len, val.data, val.len);
  if ((val = d_to_bytes(d_get(t, K_REGISTER_TIME))).data && val.len < 9) memcpy(buffer + 32 + 8 - val.len, val.data, val.len);
  if ((val = d_to_bytes(d_get(t, K_PROPS))).data && val.len < 25) memcpy(buffer + 40 + 24 - val.len, val.data, val.len);
  if ((val = d_to_bytes(d_get(t, K_WEIGHT))).data && val.len < 9) memcpy(buffer + 64 + 8 - val.len, val.data, val.len);
  if ((val = d_to_bytes(d_get(t, K_ADDRESS))).data && val.len < 21) memcpy(buffer + 72 + 20 - val.len, val.data, val.len);
  if (url.data && url.len) memcpy(buffer + 92, url.data, url.len);

  sha3_to(&data, dst);
}

static in3_ret_t verify_nodelist_data(in3_vctx_t* vc, const uint32_t node_limit, bytes_t* seed, d_token_t* required_addresses, d_token_t* server_list, d_token_t* storage_proofs) {
  bytes32_t skey, svalue;
  uint32_t  total_servers = d_get_intk(vc->result, K_TOTAL_SERVERS);

  TRY(check_storage(vc, storage_proofs, get_storage_array_key(0, 0, 0, 0, skey), as_bytes32(svalue, d_to_bytes(d_get(vc->result, K_TOTAL_SERVERS)))));

  if (node_limit && node_limit < total_servers) {
    if (d_len(server_list) != (int) node_limit) return vc_err(vc, "wrong length of the nodes!");
    const uint32_t seed_len     = required_addresses ? d_len(required_addresses) : 0;
    uint32_t *     seed_indexes = alloca(seed_len ? seed_len : 1), i = 0;

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
    uint32_t* indexes = alloca(node_limit);
    create_random_indexes(total_servers, node_limit, seed, seed_indexes, seed_len, indexes);

    // check that we have the correct indexes in the nodelist
    i = 0;
    for (d_iterator_t it = d_iter(server_list); it.left && i < node_limit; d_iter_next(&it), i++) {
      uint32_t index = d_get_intk(it.token, K_INDEX);
      if (index != indexes[i]) return vc_err(vc, "wrong index in partial nodelist");
    }
  } else if ((int) total_servers != d_len(server_list))
    return vc_err(vc, "wrong number of nodes in the serverlist");

  // now check the content of the nodelist
  for (d_iterator_t it = d_iter(server_list); it.left; d_iter_next(&it)) {
    uint32_t index = d_get_intk(it.token, K_INDEX);
    create_node_hash(it.token, svalue);
    TRY(check_storage(vc, storage_proofs, get_storage_array_key(0, index, 5, 4, skey), svalue));
  }

  return IN3_OK;
}

static in3_ret_t verify_whitelist_data(in3_vctx_t* vc, d_token_t* server_list, d_token_t* storage_proofs) {
  bytes32_t skey;
  uint32_t  total_servers = d_get_intk(vc->result, K_TOTAL_SERVERS);

  if ((int) total_servers != d_len(server_list))
    return vc_err(vc, "wrong number of nodes in the whitelist");

  // now check the content of the whitelist
  bytes32_t hash;
  bytes_t*  b = b_new(NULL, 20 * total_servers);
  int       i = 0;
  for (d_iterator_t it = d_iter(server_list); it.left; d_iter_next(&it), i += 20)
    memcpy(b->data + i, d_bytesl(it.token, 20)->data, 20);

  sha3_to(b, hash);
  b_free(b);
  TRY(check_storage(vc, storage_proofs, get_storage_array_key(0, 1, 0, 0, skey), hash));
  return IN3_OK;
}

static in3_ret_t verify_account(in3_vctx_t* vc, address_t required_contract, d_token_t** storage_proof, d_token_t** servers) {
  uint8_t         hash[32], val[36];
  bytes_t         root, **proof, *account_raw, path = {.data = hash, .len = 32};
  d_token_t *     server_list = d_get(vc->result, K_NODES), *t;
  bytes_builder_t bb          = {.bsize = 36, .b = {.data = val, .len = 0}};
  *servers                    = server_list;

  if (d_type(vc->result) != T_OBJECT || !vc->proof || !server_list) return vc_err(vc, "Invalid nodelist response!");

  // verify the header
  bytes_t* blockHeader = d_get_bytesk(vc->proof, K_BLOCK);
  if (!blockHeader) return vc_err(vc, "No Block-Proof!");
  TRY(eth_verify_blockheader(vc, blockHeader, NULL));

  // check contract
  bytes_t* contract = d_get_byteskl(vc->result, K_CONTRACT, 20);
  if (!contract || (required_contract && memcmp(contract->data, required_contract, 20)))
    return vc_err(vc, "No or wrong Contract!");

  // check last block
  if (rlp_decode_in_list(blockHeader, BLOCKHEADER_NUMBER, &root) != 1 || bytes_to_long(root.data, root.len) < d_get_longk(vc->result, K_LAST_BLOCK_NUMBER))
    return vc_err(vc, "The signature is based on older block!");

  // check accounts
  d_token_t* accounts = d_get(vc->proof, K_ACCOUNTS);
  if (!accounts || d_len(accounts) != 1)
    return vc_err(vc, "Invalid accounts!");
  d_token_t* account = accounts + 1;

  // verify the account proof
  if (rlp_decode_in_list(blockHeader, BLOCKHEADER_STATE_ROOT, &root) != 1) return vc_err(vc, "no state root in the header");
  if (!b_cmp(d_get_byteskl(account, K_ADDRESS, 20), contract)) return vc_err(vc, "wrong address in the account proof");

  proof = d_create_bytes_vec(d_get(account, K_ACCOUNT_PROOF));
  if (!proof) return vc_err(vc, "no merkle proof for the account");

  account_raw = serialize_account(account);
  sha3_to(contract, hash);
  if (!trie_verify_proof(&root, &path, proof, account_raw)) {
    _free(proof);
    b_free(account_raw);
    return vc_err(vc, "invalid account proof");
  }
  _free(proof);
  b_free(account_raw);

  // now verify storage proofs
  if (!(*storage_proof = d_get(account, K_STORAGE_PROOF))) return vc_err(vc, "no stortage-proof found!");
  if ((t = d_getl(account, K_STORAGE_HASH, 32)))
    root = *d_bytes(t);
  else
    return vc_err(vc, "no storage-hash found!");

  for (d_iterator_t it = d_iter(*storage_proof); it.left; d_iter_next(&it)) {
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

  return IN3_OK;
}

in3_ret_t eth_verify_in3_whitelist(in3_vctx_t* vc) {
  d_token_t *storage_proof = NULL, *server_list = NULL;
  in3_ret_t  res = verify_account(vc, vc->chain->whitelist ? vc->chain->whitelist->contract : NULL, &storage_proof, &server_list);

  return res == IN3_OK ? verify_whitelist_data(vc, server_list, storage_proof) : res;
}

in3_ret_t eth_verify_in3_nodelist(in3_vctx_t* vc, uint32_t node_limit, bytes_t* seed, d_token_t* required_addresses) {
  d_token_t *storage_proof = NULL, *server_list = NULL;
  in3_ret_t  res = verify_account(vc, vc->chain->contract->data, &storage_proof, &server_list);

  // now verify the nodelist
  return res == IN3_OK ? verify_nodelist_data(vc, node_limit, seed, required_addresses, server_list, storage_proof) : res;
}
