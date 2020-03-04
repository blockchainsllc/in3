

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

#ifndef TEST
#define TEST
#endif
#include "../src/core/client/context.h"
#include "../src/core/client/keys.h"
#include "../src/core/util/data.h"
#include "../src/core/util/log.h"
#include "../src/core/util/mem.h"
#include "../src/third-party/crypto/ecdsa.h"
#include "../src/third-party/crypto/secp256k1.h"
#include "../src/verifier/eth1/basic/trie.h"
#include "../src/verifier/eth1/evm/big.h"
#include "../src/verifier/eth1/evm/evm.h"
#include "../src/verifier/eth1/evm/gas.h"
#include "../src/verifier/eth1/nano/rlp.h"
#include "../src/verifier/eth1/nano/serialize.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "vm_runner.h"

static bytes_t current_block = {.data = NULL, .len = 0};
d_token_t*     vm_get_account(d_token_t* test, uint8_t* adr) {
  char tmp[44];
  tmp[0] = '0';
  tmp[1] = 'x';
  bytes_to_hex(adr, 20, tmp + 2);
  return d_get(d_get(test, key("pre")), key(tmp));
}
d_token_t* vm_get_storage(d_token_t* test, uint8_t* adr, uint8_t* k, int l_key) {
  char tmp[68];
  tmp[0] = '0';
  tmp[1] = 'x';
  bytes_to_hex(k, l_key, tmp + 2);
  return d_get(d_get(vm_get_account(test, adr), key("storage")), key(tmp));
}
static uint8_t __zero = 0, __tmp[32];

int runner_get_env(void* evm_ptr, uint16_t evm_key, uint8_t* in_data, int in_len, uint8_t** out_data, int offset, int len) {
  bytes_t res;

  d_token_t *t, *t2;
  int        i;

  evm_t* evm = evm_ptr;
  if (!evm) return EVM_ERROR_INVALID_ENV;
  d_token_t* test = evm->env_ptr;
  if (!test) return EVM_ERROR_INVALID_ENV;

  switch (evm_key) {
    case EVM_ENV_BLOCKHEADER:
      *out_data = current_block.data;
      return current_block.len;

    case EVM_ENV_BALANCE:
      t = vm_get_account(test, in_data);
      if (t) {
        res = d_to_bytes(d_get(t, K_BALANCE));
        if (res.data != NULL) {
          *out_data = res.data;
          return res.len;
        }
      }
      *out_data = &__zero;
      return 1;

    case EVM_ENV_STORAGE:
      if ((t = vm_get_storage(test, evm->address, in_data, in_len))) {
        res       = d_to_bytes(t);
        *out_data = res.data;
        return res.len;
      }
      *out_data = &__zero;
      return 1;

    case EVM_ENV_BLOCKHASH:
      return EVM_ERROR_UNSUPPORTED_CALL_OPCODE;

    case EVM_ENV_CODE_SIZE:
      t = vm_get_account(test, in_data);
      if (t) {
        res = d_to_bytes(d_get(t, K_CODE));
        if (res.data != NULL) {
          int ll = 4;
          int_to_bytes(res.len, __tmp);
          for (int i = 4; i > 0; i--) {
            if (__tmp[4 - i]) {
              *out_data = __tmp + 4 - i;
              return i;
            }
          }

          *out_data = __tmp;
          return 1;
        }
      }
      *out_data = &__zero;
      return 1;
    case EVM_ENV_NONCE:
      t = vm_get_account(test, in_data);
      if (t) {
        res = d_to_bytes(d_get(t, K_NONCE));
        if (res.data != NULL) {
          *out_data = res.data;
          return res.len;
        }
      }
      *out_data = &__zero;
      return 1;

    case EVM_ENV_CODE_HASH:
      t = vm_get_account(test, in_data);
      if (t) {
        res = d_to_bytes(d_get(t, K_CODE));
        if (res.data != NULL) {
          sha3_to(&res, __tmp);
          *out_data = __tmp;
          return 32;
        }
      }
      *out_data = &__zero;
      return 0;

    case EVM_ENV_CODE_COPY:
      t = vm_get_account(test, in_data);
      if (t) {
        res = d_to_bytes(d_get(t, K_CODE));
        if (res.data != NULL) {
          *out_data = res.data;
          return res.len;
        }
      }
      *out_data = &__zero;
      return 0;
  }
  return -2;
}
void prepare_header(d_token_t* block) {
  bytes_builder_t* rlp = bb_new();
  d_token_t *      sealed_fields, *t;
  uint8_t          data[32];
  bytes_t          tmp = {.data = data, .len = 32}, r;
  memset(data, 0, 32);
  rlp_encode_item(rlp, &tmp);
  rlp_encode_item(rlp, &tmp);
  r = d_to_bytes(d_get(block, key("currentCoinbase")));
  rlp_encode_item(rlp, &r);
  rlp_encode_item(rlp, &tmp);
  rlp_encode_item(rlp, &tmp);
  rlp_encode_item(rlp, &tmp);
  rlp_encode_item(rlp, &tmp);
  r = d_to_bytes(d_get(block, key("currentDifficulty")));
  rlp_encode_item(rlp, &r);
  r = d_to_bytes(d_get(block, key("currentNumber")));
  rlp_encode_item(rlp, &r);
  r = d_to_bytes(d_get(block, key("currentGasLimit")));
  rlp_encode_item(rlp, &r);
  rlp_encode_item(rlp, &tmp);
  r = d_to_bytes(d_get(block, key("currentTimestamp")));
  rlp_encode_item(rlp, &r);
  rlp_encode_item(rlp, &tmp);

  // clang-format on
  bytes_t* b         = bb_move_to_bytes(rlp_encode_to_list(rlp));
  current_block.data = b->data;
  current_block.len  = b->len;
  _free(b);
}

int check_post_state(evm_t* evm, d_token_t* post) {
#ifdef EVM_GAS
  int        i, j;
  d_token_t *t, *storages, *s;
  for (i = 0, t = post + 1; i < d_len(post); i++, t = d_next(t)) {
    char*   adr_str = d_get_keystr(t->key);
    uint8_t address[20];
    hex_to_bytes(adr_str + 2, strlen(adr_str) - 2, address, 20);
    storages = d_get(t, key("storage"));
    for (j = 0, s = storages + 1; j < d_len(storages); j++, t = d_next(s)) {
      char*      s_str = d_get_keystr(s->key);
      uint8_t    s_key[32];
      int        l_key    = hex_to_bytes(s_str + 2, strlen(s_str) - 2, s_key, 32);
      bytes_t    val_must = d_to_bytes(s);
      storage_t* st       = evm_get_storage(evm, address, s_key, l_key, 0);
      if (!st) {
        print_error("Missing the storage key!");
        return -1;
      }
      if (big_cmp(st->value, 32, val_must.data, val_must.len)) {
        print_error("Wrong storage value!");
        printf("expected : ");
        b_print(&val_must);
        val_must.data = st->value;
        val_must.len  = 32;
        printf("is : ");
        b_print(&val_must);
        return -1;
      }
    }
  }
#endif
  return 0;
}

bytes_t* to_uint256(bytes_t* b, uint8_t* val) {
  int l = 32;
  while (*val == 0 && l) {
    l--;
    val++;
  }
  b->data = val;
  b->len  = l;
  return b;
}

void generate_storage_hash(evm_t* evm, storage_t* s, uint8_t* dst) {

  trie_t*          trie = trie_new();
  bytes_t          tmp, k;
  bytes_builder_t* bb = bb_new();
  while (s) {
    to_uint256(&tmp, s->value);

    if (tmp.len) {
      //      bytes_t value = bytes(s->value, 32);
      bb_clear(bb);
      rlp_encode_item(bb, &tmp);
      k.len  = 32;
      k.data = s->key;
      sha3_to(&k, dst);
      k.data = dst;
      trie_set_value(trie, &k, &bb->b);
      EVM_DEBUG_BLOCK({
        to_uint256(&k, s->key);
        in3_log_trace(":::    - ");
        if (k.len)
          ba_print(k.data, k.len);
        else
          in3_log_trace(" 0x00");
        in3_log_trace(" : ");
        if (tmp.len)
          ba_print(tmp.data, tmp.len);
        else
          in3_log_trace(" 0x00");
        in3_log_trace("\n");
      });
    }

    s = s->next;
  }
  memcpy(dst, trie->root, 32);
  trie_free(trie);
  bb_free(bb);
}

bytes_t* serialize_ac(evm_t* evm, account_t* ac) {

  bytes_builder_t* rlp = bb_new();
  bytes_t          tmp = {0};
  uint8_t          hash[32];

  rlp_encode_item(rlp, to_uint256(&tmp, ac->nonce));

  EVM_DEBUG_BLOCK({
    in3_log_trace(":::   nonce   : ");
    ba_print(tmp.data, tmp.len);
  });

  rlp_encode_item(rlp, to_uint256(&tmp, ac->balance));
  EVM_DEBUG_BLOCK({
    in3_log_trace("\n:::   balance : ");
    ba_print(tmp.data, tmp.len);
    in3_log_trace("\n:::   code    : ");
    ba_print(ac->code.data, ac->code.len);
    in3_log_trace("\n:::   storage : \n");
  });

  generate_storage_hash(evm, ac->storage, hash);

  EVM_DEBUG_BLOCK({
    in3_log_trace("\n  storageHash : \n");
    ba_print(hash, 32);
    in3_log_trace("\n");
  });
  tmp.data = hash;
  tmp.len  = 32;
  rlp_encode_item(rlp, &tmp);

  sha3_to(&ac->code, hash);
  rlp_encode_item(rlp, &tmp);

  // clang-format on
  return bb_move_to_bytes(rlp_encode_to_list(rlp));
}
int num_bytes(uint8_t* val, int l) {
  optimize_len(val, l);
  return l == 1 && *val == 0 ? 0 : l;
}

int generate_state_root(evm_t* evm, uint8_t* dst) {
  trie_t*    trie = trie_new();
  uint8_t    hash[32];
  bytes_t    hash_bytes = {.data = hash, .len = 32};
  d_token_t* test       = (d_token_t*) evm->env_ptr;
#ifdef EVM_GAS
  // make sure we have all accounts
  d_token_t *accounts = d_get(test, key("pre")), *t;
  int        i;
  for (i = 0, t = accounts + 1; i < d_len(accounts); i++, t = d_next(t)) {
    uint8_t adr[20];
    hex_to_bytes(d_get_keystr(t->key) + 2, 40, adr, 20);
    evm_get_account(evm, adr, 1);
  }
  EVM_DEBUG_BLOCK({
    in3_log_trace("\n::: ================ ");
  });

  //
  account_t* ac = evm->accounts;
  while (ac) {
    if (num_bytes(ac->nonce, 32) == 0 && num_bytes(ac->balance, 32) == 0 && ac->code.len == 0) {
      ac = ac->next;
      continue;
    }

    EVM_DEBUG_BLOCK({
      in3_log_trace("\n::: Account ");
      ba_print(ac->address, 20);
      in3_log_trace("  ##\n");
    });
    bytes_t  adr = {.data = ac->address, .len = 20};
    bytes_t* b   = serialize_ac(evm, ac);

    sha3_to(&adr, hash);

    trie_set_value(trie, &hash_bytes, b);
    b_free(b);
    ac = ac->next;
  }

  memcpy(dst, trie->root, 32);
  trie_free(trie);

#endif
  return 0;
}

static void uint256_setn(uint8_t* src, uint64_t val) {
  uint8_t tmp[8];
  long_to_bytes(val, tmp);
  memset(src, 0, 24);
  memcpy(src + 24, tmp, 8);
}
static void uint256_setb(uint8_t* dst, uint8_t* data, int len) {
  if (len < 32) memset(dst, 0, 32 - len);
  memcpy(dst + 32 - len, data, len);
}

#ifdef EVM_GAS
static void read_accounts(evm_t* evm, d_token_t* accounts) {
  int        i, j;
  d_token_t *t, *storage, *s;
  for (i = 0, t = accounts + 1; i < d_len(accounts); i++, t = d_next(t)) {
    char*   adr_str = d_get_keystr(t->key);
    uint8_t address[20];
    hex_to_bytes(adr_str + 2, strlen(adr_str) - 2, address, 20);
    evm_get_account(evm, address, true);
    storage = d_get(t, key("storage"));
    if (storage) {
      for (j = 0, s = storage + 1; j < d_len(storage); j++, s = d_next(s)) {
        char*   k = d_get_keystr(s->key);
        uint8_t kk[32];
        hex_to_bytes(k + 2, strlen(k) - 2, kk, 32);
        evm_get_storage(evm, address, kk, (strlen(k) - 1) / 2, true);
      }
    }
  }
}
#endif

static d_token_t* get_test_val(d_token_t* root, char* name, d_token_t* indexes) {
  d_token_t* array = d_get(root, key(name));
  if (!array) return NULL;
  return d_get_at(array, d_get_int(indexes, strcmp(name, "gasLimit") == 0 ? "gas" : name));
}
int run_evm(d_token_t* test, uint32_t props, uint64_t* ms, char* fork_name, int test_index) {
  uint8_t caller[32];

  d_token_t* exec        = d_get(test, key("exec"));
  d_token_t* transaction = d_get(test, key("transaction"));
  d_token_t* post        = d_get(test, key("post"));
  d_token_t* indexes     = NULL;
  uint64_t   total_gas;
  address_t  _to;
  memset(_to, 0, 20);

  // create vm
  evm_t evm;
  evm.stack.b.data = _malloc(64);
  evm.stack.b.len  = 0;
  evm.stack.bsize  = 64;

  evm.memory.b.data = _calloc(32, 1);
  evm.memory.b.len  = 0;
  evm.memory.bsize  = 32;

  evm.invalid_jumpdest = NULL;

  evm.stack_size = 0;

  evm.pos   = 0;
  evm.state = EVM_STATE_INIT;

  evm.last_returned.data = NULL;
  evm.last_returned.len  = 0;

  evm.properties = props | (exec ? EVM_PROP_FRONTIER : 0); //EVM_PROP_CONSTANTINOPL;

  evm.env      = runner_get_env;
  evm.env_ptr  = test;
  evm.chain_id = 1;

  evm.return_data.data = NULL;
  evm.return_data.len  = 0;

  if (exec) {

    evm.gas_price  = d_to_bytes(d_get(exec, key("gasPrice")));
    evm.call_data  = d_to_bytes(d_get(exec, key("data")));
    evm.call_value = d_to_bytes(d_get(exec, key("value")));

    evm.caller  = d_get_bytes(exec, "caller")->data;
    evm.origin  = d_get_bytes(exec, "origin")->data;
    evm.address = d_get_bytes(exec, "address")->data;
    evm.account = d_get_bytes(exec, "address")->data;

#ifdef EVM_GAS
    evm.accounts = NULL;
    evm.gas      = d_get_long(exec, "gas");
    evm.code     = d_to_bytes(d_get(exec, K_CODE));
    evm.parent   = NULL;
    evm.logs     = NULL;
    evm.init_gas = 0;
#endif

  } else if (transaction) {
    indexes        = d_get(d_get_at(d_get(post, key(fork_name)), test_index), key("indexes"));
    evm.gas_price  = d_to_bytes(d_get(transaction, key("gasPrice")));
    evm.call_data  = d_to_bytes(get_test_val(transaction, "data", indexes));
    evm.call_value = d_to_bytes(get_test_val(transaction, "value", indexes));

    uint8_t *pk           = d_get_bytes(transaction, "secretKey")->data, public_key[65], sdata[32];
    bytes_t  pubkey_bytes = {.data = public_key + 1, .len = 64};
    ecdsa_get_public_key65(&secp256k1, pk, public_key);
    // hash it and return the last 20 bytes as address
    if (sha3_to(&pubkey_bytes, sdata) == 0)
      memcpy(caller, sdata + 12, 20);
    else
      printf("\nWrong Hash");

    evm.caller = caller;
    evm.origin = caller;

    bytes_t to_address = d_to_bytes(d_get(transaction, K_TO));
    evm.address        = _to;
    if (to_address.len) memcpy(_to, to_address.data, 20);
    //      memcpy(_to + 32 - to_address.len, to_address.data, to_address.len);
    evm.account = evm.address;

    if (d_getl(transaction, K_TO, 20) && d_len(d_getl(transaction, K_TO, 20)))
      evm.code = d_to_bytes(d_get(vm_get_account(test, d_get_bytes(transaction, "to")->data), K_CODE));
    else
      evm.code = evm.call_data;

#ifdef EVM_GAS
    evm.accounts = NULL;
    evm.gas      = d_long(get_test_val(transaction, "gasLimit", indexes));
    evm.parent   = NULL;
    evm.logs     = NULL;
    evm.refund   = 0;
    evm.init_gas = evm.gas;

    // prepare all accounts
    read_accounts(&evm, d_get(test, key("pre")));

    // we need to create an account since we don't have one
    if (big_is_zero(evm.address, 20)) {

      //  calculate the generated address
      uint8_t*         nonce = evm_get_account(&evm, caller, true)->nonce;
      bytes_builder_t* bb    = bb_new();
      bytes_t          tmp   = bytes(caller, 20);
      bytes32_t        hash;
      rlp_encode_item(bb, &tmp);
      if (big_is_zero(nonce, 32))
        tmp.len = 0;
      else {
        tmp.len  = 32;
        tmp.data = nonce;
        optimize_len(tmp.data, tmp.len);
      }
      rlp_encode_item(bb, &tmp);
      rlp_encode_to_list(bb);
      sha3_to(&bb->b, hash);
      bb_free(bb);
      memcpy(_to, hash + 12, 20);

      evm_get_account(&evm, _to, true)->nonce[31]++;
    }

    // increase the nonce and pay for gas
    account_t* c_adr = evm_get_account(&evm, evm.caller, true);
    uint256_setn(c_adr->nonce, bytes_to_long(c_adr->nonce, 32) + 1);
    uint8_t tmp[32], txval[64];
    int     l;

    // handle balance for sender
    long_to_bytes(evm.gas, tmp);
    l = big_mul(evm.gas_price.data, evm.gas_price.len, tmp, 8, txval, 32);
    l = big_add(txval, l, evm.call_value.data, evm.call_value.len, tmp, 32);
    if (big_cmp(tmp, l, c_adr->balance, 32) > 0) {
      print_error("not enough value to pay for the gas");
      evm_free(&evm);
      return 1;
    }
    l = big_sub(c_adr->balance, 32, tmp, l, txval);
    uint256_setb(c_adr->balance, txval, l);

    // handle balance for receiver
    account_t* to_adr = evm_get_account(&evm, evm.address, true);
    uint256_setb(to_adr->balance, tmp, big_add(to_adr->balance, 32, evm.call_value.data, evm.call_value.len, tmp, 32));

    // handle gas
    total_gas = G_TRANSACTION;
    for (int i = 0; i < evm.call_data.len; i++)
      total_gas += evm.call_data.data[i] ? G_TXDATA_NONZERO : G_TXDATA_ZERO;

    evm.gas = (total_gas > evm.gas) ? 0 : evm.gas - total_gas;

#endif

  } else {
    *ms = 0;
    print_error("Unknown Format - Missing transaction");
    return -1;
  }

  prepare_header(d_get(test, key("env")));

  uint64_t start = clock(), gas_before = evm.gas;
#ifdef EVM_GAS
  if (transaction && !d_len(d_get(transaction, K_TO)))
    evm.gas -= G_TXCREATE;
#endif

  int fail = evm_run(&evm, evm.account);
  *ms      = (clock() - start) / 1000;

  if (transaction) {
#ifdef EVM_GAS
    total_gas += gas_before - evm.gas;
    if (fail) {
      // it failed, so the transaction used up all the gas and we reverse all accounts
      total_gas = d_long(get_test_val(transaction, "gasLimit", indexes));
      evm.gas   = 0;
      fail      = 0;
      uint8_t    gas_tmp[32], gas_tmp2[32];
      account_t* ac = NULL;
      storage_t* s  = NULL;
      // reset all accounts except the sender
      while (evm.accounts) {
        ac = evm.accounts;
        //    if (ac->code.data) _free(ac->code.data);
        s = NULL;
        while (ac->storage) {
          s           = ac->storage;
          ac->storage = s->next;
          _free(s);
        }
        evm.accounts = ac->next;
        _free(ac);
      }

      // read the accounts from pre-state
      read_accounts(&evm, d_get(test, key("pre")));

      // reduce the gasLimit*price from caller the
      account_t* sender = evm_get_account(&evm, evm.caller, true);
      long_to_bytes(total_gas, gas_tmp);
      int l = big_mul(evm.gas_price.data, evm.gas_price.len, gas_tmp, 8, gas_tmp2, 32);
      uint256_setb(sender->balance, gas_tmp, big_sub(sender->balance, 32, gas_tmp2, l, gas_tmp));

      // incremente the nonce
      uint256_setn(sender->nonce, bytes_to_long(sender->nonce, 32) + 1);
    }

    uint8_t tmp[32], tmp2[32], eth3[8];
    int     l;

    // if there is gas left we return it to the sender
    if (evm.gas > 0) {
      account_t* c_adr = evm_get_account(&evm, evm.caller, true);
      long_to_bytes(evm.gas, tmp);
      l = big_mul(evm.gas_price.data, evm.gas_price.len, tmp, 8, tmp2, 32);
      l = big_add(tmp2, l, c_adr->balance, 32, tmp, 32);
      uint256_setb(c_adr->balance, tmp, l);
    }

    // pay the miner the total gas
    account_t* miner = evm_get_account(&evm, d_get_bytes(d_get(test, key("env")), "currentCoinbase")->data, 1);

    // increase balance of the miner
    long_to_bytes(total_gas, tmp);
    l = big_mul(evm.gas_price.data, evm.gas_price.len, tmp, 8, tmp2, 32);
    uint256_setb(miner->balance, tmp, big_add(tmp2, l, miner->balance, 32, tmp, 32));

#endif
  }

  _free(current_block.data);

  // now check results...
  if (!fail) {
    bytes_t must_out = d_to_bytes(d_get(test, key("out")));
    if (transaction) {
      uint8_t state_root[32];
      generate_state_root(&evm, state_root);
      d_token_t* pp       = d_get_at(d_get(post, key(fork_name)), test_index);
      bytes_t    expected = d_to_bytes(d_getl(pp, K_HASH, 32));
      if (pp && (expected.len != 32 || memcmp(state_root, expected.data, 32))) {
        print_error("wrong state root : ");
        ba_print(state_root, 32);
        ba_print(expected.data, 32);
        fail = 1;
      }
    } else if (must_out.len && !b_cmp(&must_out, &evm.return_data)) {
      print_error(" wrong result");
      printf("\nshould be :");
      b_print(&must_out);
      printf("\nbut is    :");
      b_print(&evm.return_data);
      fail = 1;
    } else {
      // check post state
      fail = check_post_state(&evm, post);
#ifdef EVM_GAS
      if (!fail && d_get_long(test, "gas") != evm.gas) {
        print_error("Wrong Gas");
        printf(" (expected : %" PRIu64 ", but got %" PRIu64 "", d_get_long(test, "gas"), evm.gas);
        fail = 1;
      }
#endif
    }

  } else {
    if (fail && !post) fail = 0;

    switch (fail) {
      case 0:
        break;
      case EVM_ERROR_BUFFER_TOO_SMALL:
        print_error("Memory or Buffer too small!");
        break;
      case EVM_ERROR_EMPTY_STACK:
        print_error("The Stack is empty");
        break;
      case EVM_ERROR_ILLEGAL_MEMORY_ACCESS:
        print_error("There is no Memory allocated at this position.");
        break;
      case EVM_ERROR_INVALID_ENV:
        print_error("The env could not deliver the requested value.");
        break;
      case EVM_ERROR_INVALID_JUMPDEST:
        print_error("Invalid jump destination.");
        break;
      case EVM_ERROR_INVALID_OPCODE:
        print_error("Invalid op code.");
        break;
      case EVM_ERROR_INVALID_PUSH:
        print_error("Invalid push");
        break;
      case EVM_ERROR_TIMEOUT:
        print_error("timeout running the call");
        break;
      case EVM_ERROR_UNSUPPORTED_CALL_OPCODE:
        print_error("This op code is not supported with eth_call!");
        break;
      case EVM_ERROR_OUT_OF_GAS:
        print_error("Ran out of gas.");
        break;
      default:
        printf("Unknown return-code %i", fail);
        break;
    }
  }
  evm_free(&evm);
  return fail;
}

int test_evm(d_token_t* test, uint32_t props, uint64_t* ms) {
  if (d_get(test, key("transaction"))) {
    char*      chain   = (props & EVM_PROP_CONSTANTINOPL) ? "Constantinople" : "Byzantium";
    d_token_t* results = d_get(d_get(test, key("post")), key(chain));
    if (!results) return 0;
    int res = 0, test_index;
    for (test_index = 0; res == 0 && test_index < d_len(results); test_index++)
      res = run_evm(test, props, ms, chain, test_index);
    //    if (res == 0)
    //      res = run_evm(test, props | EVM_PROP_CONSTANTINOPL, ms, "Constantinople");

    return res;
  } else
    return run_evm(test, props | EVM_PROP_FRONTIER | EVM_PROP_NO_FINALIZE, ms, NULL, 0);
}