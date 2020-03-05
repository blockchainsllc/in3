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

#include "chainspec.h"
#include "../../../core/util/log.h"
#include "../../../core/util/mem.h"
#include "../../../core/util/utils.h"
#include "chains.h"
#include "rlp.h"

// linked list of chain specs
typedef struct spec_ {
  chain_id_t    chain_id;
  chainspec_t*  spec;
  struct spec_* next;
} spec_t;

static spec_t* specs = NULL;

static void* log_error(char* msg) {
  UNUSED_VAR(msg);
  in3_log_error(msg);
  return NULL;
}

// supported EIPS
static const uint32_t EIPS[] = {145, 155, 150, 160, 170, 140, 196, 197, 198, 211, 214, 658, 145, 1014, 1052, 1283, 0}; // all const
static void           fill_aura(d_token_t* validators, consensus_transition_t* t, char* block) {
  d_token_t* contract = d_get(validators, key("contract"));
  d_token_t* list     = d_get(validators, key("list"));
  if (!contract) contract = d_get(validators, key("safeContract"));

#ifndef __ZEPHYR__
  if (block) t->transition_block = atoll(block);
#else
  if (block) t->transition_block = atoi(block);
#endif

  t->type = ETH_POA_AURA;
  if (contract) {
    t->contract = _calloc(20, 1);
    bytes_t bb  = d_to_bytes(contract);
    memcpy(t->contract + 20 - bb.len, bb.data, bb.len);
  } else
    t->contract = NULL;
  t->validators.len  = 0;
  t->validators.data = NULL;
  if (list) {
    t->validators.len  = d_len(list) * 20;
    t->validators.data = _calloc(1, t->validators.len);
    int n              = 0;
    for (d_iterator_t iter = d_iter(list); iter.left; d_iter_next(&iter)) {
      bytes_t bb = d_to_bytes(iter.token);
      memcpy(t->validators.data + 20 * (n++) + 20 - bb.len, bb.data, bb.len);
    }
  }
}

static uint64_t fill_transition(d_token_t* params, uint64_t b, eip_t* eip) {
  uint64_t   next_block = b;
  d_token_t *start, *end;
  char       tmp[200];
  memset(eip, 0, sizeof(eip_t));

  for (int n = 0; EIPS[n]; n++) {
    sprintf(tmp, "eip%iTransition", EIPS[n]);
    start = d_get(params, key(tmp));
    sprintf(tmp, "eip%iDisableTransition", EIPS[n]);
    end = d_get(params, key(tmp));
    if (start && d_long(start) > b && (next_block == b || d_long(start) < next_block)) next_block = d_long(start);
    if (end && d_long(end) > b && (next_block == b || d_long(end) < next_block)) next_block = d_long(end);
    if (start && d_long(start) <= b && (!end || d_long(end) > b)) {
      switch (EIPS[n]) {
        case 140: eip->eip140 = 1; break;
        case 145: eip->eip145 = 1; break;
        case 150:
          eip->eip150 = 1;
          break;
        case 155:
          eip->eip155 = 1;
          break;
        case 160:
          eip->eip160 = 1;
          break;
        case 170:
          eip->eip170 = 1;
          break;
        case 196: eip->eip196 = 1; break;
        case 197: eip->eip197 = 1; break;
        case 198: eip->eip198 = 1; break;
        case 211: eip->eip211 = 1; break;
        case 214: eip->eip214 = 1; break;
        case 658: eip->eip658 = 1; break;
        case 1014: eip->eip1014 = 1; break;
        case 1052: eip->eip1052 = 1; break;
        case 1283: eip->eip1283 = 1; break;
      }
    }
  }
  return next_block;
}

chainspec_t* chainspec_create_from_json(d_token_t* data) {
  chainspec_t* spec = _malloc(sizeof(chainspec_t));
  if (!spec) return log_error("not enough memory for chainspec!");
  d_token_t* params = d_get(data, key("params"));
  if (!params) return log_error("no params-tag in data");

  spec->network_id          = d_get_long(params, "networkID");
  spec->account_start_nonce = d_get_long(params, "accountStartNonce");

  // find all eip transitions
  unsigned int allocated                  = 3;
  spec->eip_transitions_len               = 1;
  spec->eip_transitions                   = _malloc(allocated * sizeof(eip_transition_t));
  spec->eip_transitions->transition_block = 0;
  memset(&spec->eip_transitions->eips, 0, sizeof(eip_t));

  for (uint64_t b = 0, next_b = fill_transition(params, 0, &spec->eip_transitions->eips); b < next_b;) {
    if (spec->eip_transitions_len == allocated) {
      spec->eip_transitions = _realloc(spec->eip_transitions, (allocated + 3) * sizeof(eip_transition_t), allocated * sizeof(eip_transition_t));
      allocated += 3;
    }
    b                                                                 = next_b;
    spec->eip_transitions[spec->eip_transitions_len].transition_block = b;
    next_b                                                            = fill_transition(params, b, &spec->eip_transitions[spec->eip_transitions_len].eips);
    spec->eip_transitions_len++;
  }

  // find consensus transitions
  spec->consensus_transitions_len               = 1;
  spec->consensus_transitions                   = _malloc(sizeof(consensus_transition_t));
  spec->consensus_transitions->transition_block = 0;
  spec->consensus_transitions->validators.len   = 0;
  spec->consensus_transitions->validators.data  = NULL;
  spec->consensus_transitions->contract         = NULL;

  d_token_t* engine = d_get(data, key("engine"));
  if (!engine) return log_error("no engine specified");
  d_token_t* genesis = d_get(data, key("genesis"));
  if (!genesis) return log_error("no genesis specified");

  if (d_get(d_get(engine, key("Ethash")), key("params")))
    spec->consensus_transitions->type = ETH_POW;
  else if ((params = d_get(d_get(d_get(engine, key("authorityRound")), key("params")), key("validators")))) {
    spec->consensus_transitions->type = ETH_POA_AURA;
    d_token_t* multi                  = d_get(params, key("multi"));
    if (multi) {
      int n                           = 0;
      spec->consensus_transitions_len = d_len(multi);
      spec->consensus_transitions     = _realloc(spec->consensus_transitions, sizeof(consensus_transition_t) * spec->consensus_transitions_len, sizeof(consensus_transition_t));
      for (d_iterator_t iter = d_iter(multi); iter.left; d_iter_next(&iter))
        fill_aura(iter.token, spec->consensus_transitions + (n++), d_get_keystr(iter.token->key));
    } else
      fill_aura(params, spec->consensus_transitions, NULL);

  } else if (d_get(d_get(engine, key("clique")), key("params"))) {
    bytes_t* extra = d_get_bytes(genesis, "extraData");
    if (!extra) return log_error("no extra data in the genesis-block");
    spec->consensus_transitions->type            = ETH_POA_CLIQUE;
    spec->consensus_transitions->validators.data = _malloc(extra->len - 32 - 65);
    spec->consensus_transitions->validators.len  = extra->len - 32 - 65;
    memcpy(spec->consensus_transitions->validators.data, extra->data + 32, spec->consensus_transitions->validators.len);
  }

  return spec;
}

eip_t chainspec_get_eip(chainspec_t* spec, uint64_t block_number) {
  for (int i = spec->eip_transitions_len - 1; i >= 0; i--) {
    if (spec->eip_transitions[i].transition_block <= block_number)
      return spec->eip_transitions[i].eips;
  }
  return spec->eip_transitions->eips;
}

consensus_transition_t* chainspec_get_consensus(chainspec_t* spec, uint64_t block_number) {
  for (int i = spec->consensus_transitions_len - 1; i >= 0; i--) {
    if (spec->consensus_transitions[i].transition_block <= block_number)
      return spec->consensus_transitions + i;
  }
  return spec->consensus_transitions;
}

static void add_rlp(bytes_builder_t* bb, uint64_t val) {
  uint8_t buffer[8];
  bytes_t tmp = {.data = buffer, .len = 8};
  long_to_bytes(val, buffer);
  b_optimize_len(&tmp);
  if (!val) tmp.len = 0;
  rlp_encode_item(bb, &tmp);
}

in3_ret_t chainspec_to_bin(chainspec_t* spec, bytes_builder_t* bb) {
  add_rlp(bb, 1); // version
  add_rlp(bb, spec->network_id);
  add_rlp(bb, spec->account_start_nonce);
  bytes_builder_t* t = bb_new();
  for (unsigned int i = 0; i < spec->eip_transitions_len; i++) {
    add_rlp(t, spec->eip_transitions[i].transition_block);
    bytes_t tmp = {.data = (uint8_t*) &spec->eip_transitions[i].eips, .len = sizeof(eip_t)};
    rlp_encode_item(t, &tmp);
  }
  rlp_encode_list(bb, &t->b);
  bb_clear(t);
  for (unsigned int i = 0; i < spec->consensus_transitions_len; i++) {
    add_rlp(t, spec->consensus_transitions[i].transition_block);
    add_rlp(t, (uint64_t) spec->consensus_transitions[i].type);
    rlp_encode_item(t, &spec->consensus_transitions[i].validators);
    bytes_t tmp = {.data = spec->consensus_transitions[i].contract, .len = spec->consensus_transitions[i].contract ? 20 : 0};
    rlp_encode_item(t, &tmp);
  }
  rlp_encode_list(bb, &t->b);
  rlp_encode_to_list(bb);
  bb_free(t);
  return IN3_OK;
}

chainspec_t* chainspec_from_bin(void* raw) {
  bytes_t data = bytes(raw, 0xFFFFFF); // since we ond't know the length we give a max size, but the length is encoded in the first bytes

  if (rlp_decode(&data, 0, &data) != 2) return log_error("invalid data");
  bytes_t      tmp, t2;
  unsigned int n;
  if (rlp_decode(&data, 0, &tmp) != 1 || tmp.len != 1 || *tmp.data != 1)
    return log_error("Invalid version");

  chainspec_t* spec = _malloc(sizeof(chainspec_t));
  if (rlp_decode(&data, 1, &tmp) != 1) return log_error("Invalid networkid");
  spec->network_id = bytes_to_long(tmp.data, tmp.len);
  if (rlp_decode(&data, 2, &tmp) != 1) return log_error("Invalid nonce");
  spec->account_start_nonce = bytes_to_long(tmp.data, tmp.len);
  if (rlp_decode(&data, 3, &tmp) != 2) return log_error("Invalid eips");
  spec->eip_transitions_len = rlp_decode_len(&tmp) >> 1;
  spec->eip_transitions     = _malloc(sizeof(eip_transition_t) * spec->eip_transitions_len);
  for (n = 0; n < spec->eip_transitions_len; n++) {
    if (rlp_decode(&tmp, n * 2, &t2) != 1) return log_error("Invalid block");
    spec->eip_transitions[n].transition_block = bytes_to_long(t2.data, t2.len);
    if (rlp_decode(&tmp, n * 2 + 1, &t2) != 1) return log_error("Invalid eips");
    memcpy(&spec->eip_transitions[n].eips, t2.data, sizeof(eip_t));
  }
  if (rlp_decode(&data, 4, &tmp) != 2) return log_error("Invalid consensus list");
  spec->consensus_transitions_len = rlp_decode_len(&tmp) / 4;
  spec->consensus_transitions     = _malloc(sizeof(consensus_transition_t) * spec->consensus_transitions_len);
  for (n = 0; n < spec->consensus_transitions_len; n++) {
    consensus_transition_t* tr = spec->consensus_transitions + n;
    if (rlp_decode(&tmp, n * 4, &t2) != 1) return log_error("Invalid block");
    tr->transition_block = bytes_to_long(t2.data, t2.len);
    if (rlp_decode(&tmp, n * 4 + 1, &t2) != 1) return log_error("Invalid type");
    tr->type = bytes_to_int(t2.data, t2.len);
    if (rlp_decode(&tmp, n * 4 + 2, &t2) != 1) return log_error("Invalid validators");
    tr->validators = t2;
    if (rlp_decode(&tmp, n * 4 + 3, &t2) != 1) return log_error("Invalid contract");
    tr->contract = t2.len == 0 ? NULL : t2.data;
  }

  return spec;
}

chainspec_t* chainspec_get(chain_id_t chain_id) {
  spec_t* s = specs;
  while (s) {
    if (s->chain_id == chain_id) return s->spec;
    s = s->next;
  }

  chainspec_t* spec = NULL;

  // not found -> lazy init
  if (chain_id == 0x2a) // KOVAN
    spec = chainspec_from_bin(CHAINSPEC_KOVAN);
  else if (chain_id == 0x1) // MAINNET
    spec = chainspec_from_bin(CHAINSPEC_MAINNET);
  else if (chain_id == 0x5) // GOERLI
    spec = chainspec_from_bin(CHAINSPEC_GOERLI);

  if (spec) {
    s           = _malloc(sizeof(spec_t));
    s->chain_id = chain_id;
    s->next     = specs;
    s->spec     = spec;
    specs       = s;
  }
  return spec;
}
void chainspec_put(chain_id_t chain_id, chainspec_t* spec) {
  spec_t* s = specs;
  while (s) {
    if (s->chain_id == chain_id) {
      s->spec = spec;
      return;
    }
    s = s->next;
  }
  s           = _malloc(sizeof(spec_t));
  s->chain_id = chain_id;
  s->next     = specs;
  s->spec     = spec;
  specs       = s;
}
