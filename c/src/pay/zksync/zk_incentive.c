/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
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
#include "../../core/client/context_internal.h"
#include "../../core/client/keys.h"
#include "../../core/util/debug.h"
#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/secp256k1.h"
#include "../../third-party/zkcrypto/lib.h"
#include "zk_helper.h"
#include "zksync.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define ERROR_PAYMENT_REQUIRED -33005

static char* get_payment_data(in3_ctx_t* ctx) {
  for (cache_entry_t* ce = ctx->cache; ce; ce = ce->next) {
    if (ce->props & CACHE_PROP_PAYMENT) return (char*) (void*) ce->value.data;
  }
  return NULL;
}

in3_ret_t zksync_add_payload(in3_pay_payload_ctx_t* ctx) {
  // we only use this, if we also have a request signer
  if (ctx->ctx->client->plugin_acts & PLGN_ACT_PAY_SIGN_REQ) {
    sb_add_chars(ctx->sb, ",\"payType\":\"zksync\"");
    char* payment_data = get_payment_data(ctx->ctx);
    if (payment_data)
      sb_add_chars(ctx->sb, payment_data);
  }
  return IN3_OK;
}

static in3_ret_t ensure_payment_data(in3_ctx_t* ctx, zksync_config_t* conf) {
  // do we have a sync_key and account already?
  if (!memiszero(conf->sync_key, 32)) return IN3_OK;
  uint8_t pub[65];
  bytes_t pubkey_bytes = {.len = 64, .data = ((uint8_t*) &pub) + 1};
  char*   message      = "\x19"
                  "Ethereum Signed Message:\n68"
                  "Access zkSync account.\n\nOnly sign this message for a trusted client!";

  in3_pay_sign_req_ctx_t sctx      = {.ctx = ctx, .request = NULL, .signature = {0}};
  bytes_t                sig_bytes = bytes(sctx.signature, 65);
  keccak(bytes((void*) message, strlen(message)), sctx.request_hash);
  TRY(in3_plugin_execute_first(ctx, PLGN_ACT_PAY_SIGN_REQ, &sctx))
  if (sig_bytes.len == 65 && sig_bytes.data[64] < 2) sig_bytes.data[64] += 27;

  // copy sync_key based on signature as seed
  zkcrypto_pk_from_seed(sig_bytes, conf->sync_key);

  // determine address
  if (ecdsa_recover_pub_from_sig(&secp256k1, pub, sig_bytes.data, sctx.request_hash, sig_bytes.data[64] >= 27 ? sig_bytes.data[64] - 27 : sig_bytes.data[64]))
    return ctx_set_error(ctx, "Invalid Signature", IN3_EINVAL);
  keccak(pubkey_bytes, sctx.request_hash);
  memcpy(conf->account, sctx.request_hash + 12, 20);

  return IN3_OK;
}

static in3_ret_t set_amount(zk_fee_t* dst, in3_ctx_t* ctx, d_token_t* t) {
  if (!t) return ctx_set_error(ctx, "No value set", IN3_EINVAL);
#ifdef ZKSYNC_256
  bytes_t tmp = d_to_bytes(t);
  memset(*dst, 0, 32);
  memcpy(*dst + 32 - tmp.len, tmp.data, tmp.len);
#else
  *dst = d_long(t);
#endif
  return IN3_OK;
}

static in3_ret_t get_payed_addresses(in3_ctx_t* ctx, bytes_t* dst) {
  in3_cache_ctx_t c = {.content = NULL, .ctx = ctx, .key = alloca(20)};
  sprintf(c.key, "payed_%d", (uint32_t) ctx->client->chain.chain_id);
  TRY(in3_plugin_execute_first_or_none(ctx, PLGN_ACT_CACHE_GET, &c))
  if (c.content) {
    *dst = *c.content;
    _free(c.content);
  }
  return IN3_OK;
}

static in3_ret_t update_payed_addresses(in3_ctx_t* ctx, unsigned int nodes, bytes_t payed) {
  in3_cache_ctx_t c = {.content = &payed, .ctx = ctx, .key = alloca(20)};
  sprintf(c.key, "payed_%d", (uint32_t) ctx->client->chain.chain_id);
  TRY(in3_plugin_execute_first_or_none(ctx, PLGN_ACT_CACHE_SET, &c))

  sb_t sb = {0};
  if (nodes > payed.len / 20)
    sb_add_chars(&sb, "{\"preselect_nodes\":null}");
  else {
    sb_add_rawbytes(&sb, "{\"preselect_nodes\":\"0x", payed, 0);
    sb_add_chars(&sb, "\"}");
  }
  in3_configure_ctx_t cctx = {.client = ctx->client, .json = parse_json(sb.data), .token = NULL, .error_msg = NULL};
  cctx.token               = cctx.json->result + 1;
  in3_ret_t ret            = in3_plugin_execute_first_or_none(ctx, PLGN_ACT_CONFIG_SET, &cctx);
  if (ret && ret != IN3_EIGNORE) ctx_set_error(ctx, cctx.error_msg ? cctx.error_msg : "Could not update the preselect nodelist", ret);
  if (cctx.error_msg) _free(cctx.error_msg);
  json_free(cctx.json);
  _free(sb.data);
  return ret;
}

static in3_ret_t find_acceptable_offer(in3_ctx_t* ctx, pay_criteria_t* criteria, d_token_t* offer, d_token_t** dst_offer, d_token_t** dst_price) {

  d_token_t* price_list = d_get(offer, key("priceList"));
  if (!price_list || d_type(price_list) != T_ARRAY || d_len(price_list) < 0) return ctx_set_error(ctx, "no pricelist in the offer", IN3_ERPC);

  // find a acceptable offer
  d_token_t* price          = NULL;
  d_token_t* selected_offer = NULL;
  for (d_iterator_t offer_iter = d_iter(price_list); offer_iter.left; d_iter_next(&offer_iter)) {
    price = NULL;
    for (d_iterator_t p_iter = d_iter(d_get(offer_iter.token, key("price"))); p_iter.left; d_iter_next(&p_iter)) {
      char* token = d_get_stringk(p_iter.token, key("token"));
      if (token && strcmp(token, criteria->token ? criteria->token : "ETH") == 0) {
        price = p_iter.token;
        break;
      }
    }

    if (price) {
      uint64_t amount       = d_get_longk(offer_iter.token, key("amount"));
      uint64_t price_amount = d_get_longk(price, key("amount"));
      if (!amount) return ctx_set_error(ctx, "no amount defined in offer from node", IN3_ERPC);
      if (!price_amount) return ctx_set_error(ctx, "no price defined in offer from node", IN3_ERPC);
      if (!criteria->max_price_per_hundred_igas || ((price_amount * 100) / amount) <= criteria->max_price_per_hundred_igas) {
        selected_offer = offer_iter.token;
        break;
      }
    }
  }

  if (!selected_offer) return ctx_set_error(ctx, "no accetable offer found in node response", IN3_ERPC);

  *dst_offer = selected_offer;
  *dst_price = price;
  return IN3_OK;
}

static in3_ret_t add_to_payed_nodelist(in3_ctx_t* ctx, address_t address, unsigned int nodelen) {
  bytes_t payed_addresses = bytes(NULL, 0);
  TRY(get_payed_addresses(ctx, &payed_addresses))
  uint8_t* addresses = alloca(payed_addresses.len + 20);
  if (payed_addresses.data) {
    memcpy(addresses, payed_addresses.data, payed_addresses.len);
    _free(payed_addresses.data);
  }

  for (unsigned int i = 0; i < payed_addresses.len; i += 20) {
    if (memcmp(address, addresses + i, 20) == 0) return IN3_OK;
  }

  if (payed_addresses.len) {
    memcpy(addresses + payed_addresses.len, address, 20);
    TRY(update_payed_addresses(ctx, nodelen, bytes(addresses, payed_addresses.len + 20)))
  }
  else
    TRY(update_payed_addresses(ctx, nodelen, bytes(address, 20)))

  return IN3_OK;
}

in3_ret_t zksync_check_payment(zksync_config_t* conf, in3_pay_followup_ctx_t* ctx) {
  if (!ctx->resp_error || d_get_intk(ctx->resp_error, K_CODE) != ERROR_PAYMENT_REQUIRED || get_payment_data(ctx->ctx)) return IN3_OK;

  // the server wants payment
  d_token_t* offer = d_get(ctx->resp_error, key("offer"));
  if (!offer) return ctx_set_error(ctx->ctx, "A payment rejection without an offer", IN3_ERPC);

  // TODO right now we always accept any offer within the range if it matches the config
  pay_criteria_t* criteria = conf->incentive;
  if (!criteria) return ctx_set_error(ctx->ctx, "No Payment configuration set in zksync.incentive", IN3_ECONFIG);

  d_token_t* price          = NULL;
  d_token_t* selected_offer = NULL;
  TRY(find_acceptable_offer(ctx->ctx, criteria, offer, &selected_offer, &price))
  TRY(ensure_payment_data(ctx->ctx, &criteria->config))

  // now prepare the payment
  criteria->config.account_id = d_get_longk(offer, key("accountId"));
  criteria->config.nonce      = d_get_longk(offer, key("nonce"));
  criteria->config.sign_type  = ZK_SIGN_PK;

  // prepare the token-struct
  zksync_token_t _token = {
      .id       = d_get_intk(price, K_ID),
      .decimals = d_get_intk(price, key("decimals"))};
  strncpy(_token.symbol, d_get_stringk(price, key("token")), 6);
  bytes_t tmp = d_to_bytes(d_get(price, K_ADDRESS));
  if (tmp.len == 20)
    memcpy(_token.address, tmp.data, 20);
  else
    memset(_token.address, 0, 20);

  // create tx
  zksync_tx_data_t tx = {
      .account_id = criteria->config.account_id,
      .conf       = &criteria->config,
      .nonce      = (uint32_t) criteria->config.nonce,
      .token      = &_token,
      .type       = ZK_TRANSFER};
  TRY(set_amount(&tx.amount, ctx->ctx, d_get(price, key("amount"))))
  TRY(set_amount(&tx.fee, ctx->ctx, d_get(price, key("fee"))))
  tmp = d_to_bytes(d_get(selected_offer, K_ADDRESS));
  if (tmp.len != 20) return ctx_set_error(ctx->ctx, "invalid address in offer", IN3_ERPC);
  memcpy(tx.to, tmp.data, 20);
  memcpy(tx.from, criteria->config.account, 20);

  // prepare the payTx
  sb_t sb = {0};
  sb_add_chars(&sb, ",\"payTx\":{\"offer_id\":");
  sb_add_int(&sb, d_get_intk(selected_offer, K_ID));
  sb_add_chars(&sb, ",\"method\":\"tx_submit\",\"params\":[");

  // sign tx
  in3_ret_t ret = zksync_sign_transfer(&sb, &tx, ctx->ctx, &criteria->config);
  if (ret) {
    _free(sb.data);
    return ret;
  }
  sb_add_chars(&sb, "]}");

  // add data as cache, so we can add it when we create the next payload.
  in3_cache_add_entry(&ctx->ctx->cache, bytes(NULL, 0), bytes((void*) sb.data, strlen(sb.data)))->props = CACHE_PROP_MUST_FREE | CACHE_PROP_PAYMENT;

  // now we make sure we try again.
  TRY(in3_retry_same_node(ctx->ctx))

  TRY(add_to_payed_nodelist(ctx->ctx, ctx->node->address, criteria->payed_nodes))

  // we use IN3_WAITING, which causes it to cancel verification and try again.
  return IN3_WAITING;
}
