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
#include "../../../core/util/data.h"
#include "../../../core/util/mem.h"
#include "../../../third-party/crypto/bignum.h"
#include "../../../verifier/eth1/nano/eth_nano.h"
#include "../../../verifier/eth1/nano/merkle.h"
#include "../../../verifier/eth1/nano/rlp.h"
#include "../../../verifier/eth1/nano/serialize.h"
#include <string.h>

static uint8_t* EMPTY_HASH      = (uint8_t*) "\xc5\xd2\x46\x01\x86\xf7\x23\x3c\x92\x7e\x7d\xb2\xdc\xc7\x03\xc0\xe5\x00\xb6\x53\xca\x82\x27\x3b\x7b\xfa\xd8\x04\x5d\x85\xa4\x70";
static uint8_t* EMPTY_ROOT_HASH = (uint8_t*) "\x56\xe8\x1f\x17\x1b\xcc\x55\xa6\xff\x83\x45\xe6\x92\xc0\xf8\x6e\x5b\x48\xe0\x1b\x99\x6c\xad\xc0\x01\x62\x2f\xb5\xe3\x63\xb4\x21";
static int      is_not_existened(d_token_t* account) {
  d_token_t* t = NULL;
  // TODO: how do I determine the default nonce? It is in the chain-config
  return ((t = d_get(account, K_BALANCE)) && d_type(t) == T_INTEGER && d_int(t) == 0 && (t = d_getl(account, K_CODE_HASH, 32)) && memcmp(t->data, EMPTY_HASH, 32) == 0 && d_get_longk(account, K_NONCE) == 0) && (t = d_getl(account, K_STORAGE_HASH, 32)) && memcmp(t->data, EMPTY_ROOT_HASH, 32) == 0;
}

static in3_ret_t verify_proof(in3_vctx_t* vc, bytes_t* header, d_token_t* account) {
  d_token_t *     t, *storage_proof, *p;
  int             i;
  uint8_t         hash[32], val[36];
  bytes_t**       proof;
  bytes_t *       tmp, root, *account_raw, path = {.data = hash, .len = 32};
  bytes_builder_t bb = {.bsize = 36, .b = {.data = val, .len = 0}};

  // verify the account proof
  if (rlp_decode_in_list(header, BLOCKHEADER_STATE_ROOT, &root) != 1) return vc_err(vc, "no state root in the header");

  if ((tmp = d_get_byteskl(account, K_ADDRESS, 20)))
    sha3_to(tmp, hash);
  else
    return vc_err(vc, "no address in the account");

  proof = d_create_bytes_vec(d_get(account, K_ACCOUNT_PROOF));
  if (!proof) return vc_err(vc, "no merkle proof for the account");
  account_raw = serialize_account(account);

  if (!trie_verify_proof(&root, &path, proof, is_not_existened(account) ? NULL : account_raw)) {
    _free(proof);
    b_free(account_raw);
    return vc_err(vc, "invalid account proof");
  }
  _free(proof);
  b_free(account_raw);

  // now we verify the storage proofs
  if (!(storage_proof = d_get(account, K_STORAGE_PROOF))) return vc_err(vc, "no stortage-proof found!");

  if ((t = d_getl(account, K_STORAGE_HASH, 32)))
    root = *d_bytes(t);
  else
    return vc_err(vc, "no storage-hash found!");

  //                                "storageHash": "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
  bool is_empty = memcmp(root.data, EMPTY_ROOT_HASH, 32) == 0;

  for (i = 0, p = storage_proof + 1; i < d_len(storage_proof); i++, p = d_next(p)) {
    d_token_t* pt = d_get(p, K_PROOF);
    bb.b.len      = d_bytes_to(d_get(p, K_VALUE), val, 32);
    if (is_empty) {
      uint8_t* vp = val;
      optimize_len(vp, bb.b.len);
      if (bb.b.len > 1 || (bb.b.len == 1 && *vp))
        return vc_err(vc, "empty storagehash, so we exepct 0 values");
      if (d_type(pt) != T_ARRAY || d_len(pt) != 1 || d_type(pt + 1) != T_INTEGER || d_int(pt + 1) != 0x80)
        return vc_err(vc, "invalid proof");
    } else {

      d_bytes_to(d_get(p, K_KEY), hash, 32);
      sha3_to(&path, hash);

      proof = d_create_bytes_vec(pt);
      if (!proof) return vc_err(vc, "no merkle proof for the storage");

      // rlp encode the value.
      if (bb.b.len) {
        // remove leading zeros!
        uint8_t*     pp = bb.b.data;
        uint_fast8_t l  = 32;
        optimize_len(pp, l);
        if (l == 0 || (l == 1 && *pp == 0))
          bb.b.len = 0;
        else {
          if (pp != bb.b.data) memmove(bb.b.data, pp, l);
          bb.b.len = l;
          rlp_encode_to_item(&bb);
        }
      }

      if (!trie_verify_proof(&root, &path, proof, bb.b.len ? &bb.b : NULL)) {
        _free(proof);
        return vc_err(vc, "invalid storage proof");
      }
      _free(proof);
    }
  }

  return IN3_OK;
}

in3_ret_t eth_verify_account_proof(in3_vctx_t* vc) {

  d_token_t *t, *accounts, *contract = NULL, *proofed_account = NULL;
  char*      method = d_get_stringk(vc->request, K_METHOD);
  bytes_t    tmp;
  uint8_t    hash[32];
  int        i;

  // no result -> nothing to verify
  if (!vc->result) return IN3_OK;
  if (!vc->proof) {
    printf("Missing proof for %s\n", method);
    return vc_err(vc, "no proof");
  }

  // verify header
  bytes_t* header = d_get_bytesk(vc->proof, K_BLOCK);
  if (!header) return vc_err(vc, "no blockheader");
  if (eth_verify_blockheader(vc, header, NULL)) return vc_err(vc, "invalid blockheader");

  // make sure we blockheader is based on the right blocknumber (unless it is a nodelist or a 'latest'-string)
  t = d_get(vc->request, K_PARAMS);
  if (strcmp(method, "in3_nodeList") && (t = d_get_at(t, d_len(t) - 1)) && d_type(t) == T_INTEGER && rlp_decode_in_list(header, BLOCKHEADER_NUMBER, &tmp) == 1 && bytes_to_long(tmp.data, tmp.len) != d_long(t))
    return vc_err(vc, "the blockheader has the wrong blocknumber");

  // get the account this proof is based on
  if (!(contract = strcmp(method, "in3_nodeList") == 0 ? d_get(vc->result, K_CONTRACT) : d_get_at(d_get(vc->request, K_PARAMS), 0)))
    return vc_err(vc, "no account found in request");
  if (strcmp(method, "eth_call") == 0)
    contract = d_getl(d_get_at(d_get(vc->request, K_PARAMS), 0), K_TO, 20);

  //now check the results
  if (!(accounts = d_get(vc->proof, K_ACCOUNTS))) return vc_err(vc, "no accounts");
  for (i = 0, t = accounts + 1; i < d_len(accounts); i++, t = d_next(t)) {
    if (verify_proof(vc, header, t))
      return vc_err(vc, "failed verifying the account");
    else if (proofed_account == NULL && d_eq(contract, d_getl(t, K_ADDRESS, 20)))
      proofed_account = t;
  }

  if (!proofed_account) return vc_err(vc, "the contract this proof is based on was not part of the proof");

  if (strcmp(method, "eth_getBalance") == 0) {
    if (!d_eq(vc->result, d_get(proofed_account, K_BALANCE)))
      return vc_err(vc, "the balance in the proof is different");
  } else if (strcmp(method, "eth_getTransactionCount") == 0) {
    if (!d_eq(vc->result, d_get(proofed_account, K_NONCE)))
      return vc_err(vc, "the nonce in the proof is different");
  } else if (strcmp(method, "eth_getCode") == 0) {
    bytes_t data = d_to_bytes(vc->result);
    if (data.len) {
      if (sha3_to(&data, hash) != 0 || memcmp(d_get_byteskl(proofed_account, K_CODE_HASH, 32)->data, hash, 32))
        return vc_err(vc, "the codehash in the proof is different");
    } else if (memcmp(d_get_byteskl(proofed_account, K_CODE_HASH, 32)->data, EMPTY_HASH, 32)) // must be empty
      return vc_err(vc, "the code must be empty");
  } else if (strcmp(method, "eth_getStorageAt") == 0) {
    uint8_t result[32], proofed_result[32];
    d_bytes_to(vc->result, result, 32);
    d_token_t* storage = d_get(proofed_account, K_STORAGE_PROOF);
    d_token_t* skey    = d_get_at(d_get(vc->request, K_PARAMS), 1);

    for (i = 0, t = storage + 1; i < d_len(storage); i++, t = d_next(t)) {
      if (d_eq(skey, d_get(t, K_KEY))) {
        d_bytes_to(d_get(t, K_VALUE), proofed_result, 32);
        if (memcmp(result, proofed_result, 32) == 0)
          return IN3_OK;
        break;
      }
    }
    return vc_err(vc, "the storage result does not match");
  } else if (strcmp(method, "eth_call") == 0) {
    return IN3_OK;
  } else
    return vc_err(vc, "not supported method");

  /*

    switch (request.method) {
    case 'eth_getBalance':
      if (!toBN(value).eq(toBN(accountProof.balance))) throw new Error('The Balance does not match the one in the proof')
      break
    case 'eth_getStorageAt':
      checkStorage(accountProof, bytes32(request.params[1]), bytes32(value))
      break
    case 'eth_getCode':
      if (!bytes32(accountProof.codeHash).equals(util.keccak(value))) throw new Error('The codehash in the proof does not match the code')
      break
    case 'eth_getTransactionCount':
      if (!toBN(accountProof.nonce).eq(toBN(value))) throw new Error('The nonce in the proof does not match the returned')
      break
    case 'in3_nodeList':
      verifyNodeListData(value as ServerList, headerProof.proof, block, request)
      // the contract must be checked later in the updateList -function
      break
    default:
      throw new Error('Unsupported Account-Proof for ' + request.method)
  }

*/

  return IN3_OK;
}
