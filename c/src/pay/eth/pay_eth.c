/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
 * 
 * Copyright (C) 2018-2019 slock.it GmbH, Blockchains LLC
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
#include "pay_eth.h"
#include "../../core/client/context.h"
#include "../../core/client/keys.h"
#include "../../core/util/debug.h"
#include "../../core/util/mem.h"
#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/secp256k1.h"
#include "../../verifier/eth1/nano/eth_nano.h"
#include "../../verifier/eth1/nano/serialize.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

uint64_t calc_request_units(in3_ctx_t* ctx) {
  return ctx->len;
}

in3_ret_t pay_eth_follow_up(in3_pay_eth_t* data, in3_pay_followup_ctx_t* plugin_ctx) {
  in3_ctx_t* ctx = plugin_ctx->ctx;
  d_token_t *pay = d_get(plugin_ctx->resp_in3, key("pay")), *t;
  if (!pay || !ctx) return IN3_OK;

  // TODO Is this a attackvector? We are trusting the nonce without proof, which means if a node sends a
  // nonce as nonce +2, the node would not get any payment. But also other would not get any payment either.
  if ((t = d_get(pay, key("gasPrice")))) data->gas_price = d_long(t);
  if ((t = d_get(pay, key("nonce")))) data->nonce = d_long(t);

  // fixme 666: interaction with nodelist?
  //  node_match_t* node = plugin_ctx->node;
  //  if (node->weight) {
  //    if ((t = d_get(pay, key("payed")))) node->weight->payed = d_long(t);
  //    if ((t = d_get(pay, key("price")))) node->weight->price = d_long(t);
  //    if (plugin_ctx->resp_error && d_get_intk(plugin_ctx->resp_error, K_CODE) == IN3_EPAYMENT_REQUIRED) {
  //      // TODO now we need to decide whether it's worth to pay
  //      if (node->weight->price && (data->max_price == 0 || node->weight->price < data->max_price))
  //        return IN3_WAITING;
  //    }
  //  }

  return IN3_OK;
}

in3_ret_t pay_eth_prepare(in3_pay_eth_t* data, in3_ctx_t* ctx) {
  if (data == NULL || ctx == NULL) return IN3_EINVAL;
  return IN3_OK;
}

static void create_signed_tx(in3_pay_eth_t* data, bytes32_t key, sb_t* sb, address_t to, uint64_t _value, uint64_t chainId) {
  bytes32_t buffer;
  bytes_t   nonce = bytes(buffer, 8), gas_price = bytes(buffer + 8, 8), gas_limit = bytes(buffer + 16, 8), value = bytes(buffer + 24, 8);
  uint8_t   sig[65];
  long_to_bytes(_value, value.data);
  long_to_bytes(data->nonce, nonce.data);
  long_to_bytes(data->gas_price, gas_price.data);
  long_to_bytes(21000, gas_limit.data);
  // create raw without signature
  bytes_t* raw = serialize_tx_raw(nonce, gas_price, gas_limit, bytes(to, 20), value, bytes(NULL, 0), chainId, bytes(NULL, 0), bytes(NULL, 0));

  // sign it
  if (ecdsa_sign(&secp256k1, HASHER_SHA3K, key, raw->data, raw->len, sig, sig + 64, NULL) < 0) return;

  b_free(raw);
  raw = serialize_tx_raw(nonce, gas_price, gas_limit, bytes(to, 20), value, bytes(NULL, 0), 27 + sig[64] + (chainId ? (chainId * 2 + 8) : 0), bytes(sig, 32), bytes(sig + 32, 32));
  sb_add_bytes(sb, NULL, raw, 1, false);
  b_free(raw);
}

in3_ret_t pay_eth_handle_request(in3_pay_eth_t* data, in3_pay_handle_ctx_t* plugin_ctx) {
  in3_ctx_t*     ctx     = plugin_ctx->ctx;
  const uint64_t units   = calc_request_units(ctx);
  bool           started = false;
  sb_t*          sb      = plugin_ctx->payload;

  // fixme 666: interaction with nodelist?
  //  // TODO if we have a request Count > 1 we are currently still sending the same payload,
  //  // but for payments, we may need different payload for each.
  //  for (node_match_t* node = ctx->nodes; node; node = node->next) {
  //    if (node->weight) {
  //      if (node->weight->payed < units && node->weight->price && ctx->client->key) {
  //        // TODO we don't even know if we have enough balance, we simply give it a try.
  //        // TODO we should support ERC20-tokens instead of ether!
  //
  //        if (!started)
  //          sb_add_chars(sb, ",\"pay\":{ \"type\":\"eth\", \"tx\":[");
  //        else
  //          sb_add_char(sb, ',');
  //        started = true;
  //
  //        uint64_t val = data->bulk_size * node->weight->price;
  //        uint64_t v   = ctx->client->chain.chain_id;
  //        if (v > 0xFF) v = 0; // this is only valid for ethereum chains.
  //        create_signed_tx(ctx->client->key, sb, node->node->address->data, data->bulk_size * node->weight->price, data, v);
  //        data->nonce++;
  //        node->weight->payed += val;
  //      }
  //    }
  //  }

  if (started) sb_add_chars(sb, "]}");
  return IN3_OK;
}
static in3_ret_t config_set(in3_pay_eth_t* data, in3_configure_ctx_t* ctx) {
  char*       res   = NULL;
  json_ctx_t* json  = ctx->json;
  d_token_t*  token = ctx->token;

  if (token->key == key("bulkSize")) {
    EXPECT_TOK_U64(token);
    data->bulk_size = d_long(token);
  }
  else if (token->key == key("maxPrice")) {
    EXPECT_TOK_U64(token);
    data->max_price = d_long(token);
  }
  else {
    return IN3_EIGNORE;
  }

cleanup:
  ctx->error_msg = res;
  return ctx->error_msg ? IN3_ECONFIG : IN3_OK;
}

static in3_ret_t config_get(in3_pay_eth_t* data, in3_get_config_ctx_t* ctx) {
  sb_t* sb = ctx->sb;
  add_uint(sb, ',', "bulkSize", data->bulk_size);
  add_uint(sb, ',', "maxPrice", data->max_price);
  return IN3_OK;
}

in3_ret_t in3_pay_eth(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  in3_pay_eth_t* data = plugin_data;
  switch (action) {
    case PLGN_ACT_TERM:
      _free(data);
      return IN3_OK;
    case PLGN_ACT_CONFIG_SET:
      return config_set(data, (in3_configure_ctx_t*) plugin_ctx);
    case PLGN_ACT_CONFIG_GET:
      return config_get(data, (in3_get_config_ctx_t*) plugin_ctx);
    case PLGN_ACT_PAY_PREPARE:
      return pay_eth_prepare(data, plugin_ctx);
    case PLGN_ACT_PAY_FOLLOWUP:
      return pay_eth_follow_up(data, plugin_ctx);
    case PLGN_ACT_PAY_HANDLE:
      return pay_eth_handle_request(data, plugin_ctx);
    default: break;
  }
  return IN3_EIGNORE;
}

in3_ret_t in3_register_pay_eth(in3_t* c) {
  in3_pay_eth_t* data = _calloc(1, sizeof(*data));
  data->bulk_size     = 1000;
  data->max_price     = 10;
  return in3_plugin_register(c, PLGN_ACT_TERM | PLGN_ACT_CONFIG_SET | PLGN_ACT_CONFIG_GET | PLGN_ACT_PAY_PREPARE | PLGN_ACT_PAY_FOLLOWUP | PLGN_ACT_PAY_HANDLE, in3_pay_eth, data, false);
}
