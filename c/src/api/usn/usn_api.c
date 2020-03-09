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
#include "usn_api.h"
#include "../../core/client/context.h"
#include "../../core/client/keys.h"
#include "../../core/util/debug.h"
#include "../../core/util/mem.h"
#include "../../verifier/eth1/nano/eth_nano.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define K_MSG_TYPE key("msgType")
#define K_URL key("url")
#define K_ACTION key("action")
#define K_TRANSACTIONHASH key("transactionHash")
#define K_SIGNATURE key("signature")
#ifdef ERR_MSG
#define reject_if(c, m)            \
  if (c) {                         \
    if (parsed) json_free(parsed); \
    result.error_msg = m;          \
    result.action    = NULL;       \
    return result;                 \
  }
#define rejectp_if(c, m)      \
  if (c) {                    \
    result->error_msg = m;    \
    result->action    = NULL; \
    goto clean;               \
  }
#else
#define reject_if(c, m)            \
  if (c) {                         \
    if (parsed) json_free(parsed); \
    result.error_msg = "E";        \
    result.action    = NULL;       \
    return result;                 \
  }
#define rejectp_if(c, m)      \
  if (c) {                    \
    result->error_msg = "E";  \
    result->action    = NULL; \
    goto clean;               \
  }
#endif

static d_token_t* get_rented_event(d_token_t* receipt) {
  bytes32_t event_hash;
  hex_to_bytes("9123e6a7c5d144bd06140643c88de8e01adcbb24350190c02218a4435c7041f8", 64, event_hash, 32);
  for (d_iterator_t iter = d_iter(d_get(receipt, K_LOGS)); iter.left; d_iter_next(&iter)) {
    bytes_t* t = d_bytesl(d_get_at(d_get(iter.token, K_TOPICS), 0), 32);
    if (t && t->len == 32 && memcmp(event_hash, t->data, 32) == 0) return iter.token;
  }
  return NULL;
}

static usn_device_t* find_device(usn_device_conf_t* conf, char* url) {
  if (!url) return NULL;
  for (int i = 0; i < conf->len_devices; i++) {
    if (strcmp(url, conf->devices[i].url) == 0) return conf->devices + i;
  }
  return NULL;
}

static usn_device_t* find_device_by_id(usn_device_conf_t* conf, bytes32_t id) {
  if (!id) return NULL;
  for (int i = 0; i < conf->len_devices; i++) {
    if (memcmp(id, conf->devices[i].id, 32) == 0) return conf->devices + i;
  }
  return NULL;
}

static in3_ret_t exec_eth_call(usn_device_conf_t* conf, char* fn_hash, bytes32_t device_id, bytes_t data, uint8_t* result, int max) {
  int      l     = 4 + 32 + data.len;
  uint8_t* cdata = alloca(l);
  hex_to_bytes(fn_hash, -1, cdata, 4);
  memcpy(cdata + 4, device_id, 32);
  if (data.len) memcpy(cdata + 36, data.data, data.len);

  char* args = alloca(l * 2 + 100);
  char *op = args, *p = (char*) args + sprintf((char*) args, "[{\"data\":\"0x");
  p += bytes_to_hex(cdata, l, p);
  p += sprintf(p, "\",\"gas\":\"0x77c810\",\"to\":\"0x");
  p += bytes_to_hex(conf->contract, 20, p);
  sprintf(p, "\"},\"latest\"]");

  // send the request
  in3_ctx_t* ctx = in3_client_rpc_ctx(conf->c, "eth_call", op);

  // do we have a valid result?
  in3_ret_t res = ctx_get_error(ctx, 0);
  if (res != IN3_OK) {
    ctx_free(ctx);
    return res;
  }
  l = d_bytes_to(d_get(ctx->responses[0], K_RESULT), result, max);
  ctx_free(ctx);
  return l == max ? l : IN3_EINVALDT;
}

static in3_ret_t exec_eth_send(usn_device_conf_t* conf, bytes_t data, bytes32_t value, bytes32_t tx_hash) {
  char* args = alloca((4 + 32 + data.len) * 2 + 200);
  char *op = args, *p = (char*) args + sprintf((char*) args, "[{\"data\":\"0x");
  p += bytes_to_hex(data.data, data.len, p);
  p += sprintf(p, "\",\"gasLimit\":\"0x0f4240\",\"to\":\"0x");
  p += bytes_to_hex(conf->contract, 20, p);
  if (value) {
    uint8_t *vs = value, vl = 32;
    optimize_len(vs, vl);
    if (vl > 1 || *vs) {
      p += sprintf(p, "\",\"value\":\"0x");
      p += bytes_to_hex(vs, vl, p);
    }
  }
  sprintf(p, "\"}]");

  // send the request
  in3_ctx_t* ctx = in3_client_rpc_ctx(conf->c, "eth_sendTransaction", op);

  // do we have a valid result?
  in3_ret_t res = ctx_get_error(ctx, 0);
  if (res != IN3_OK) {
    ctx_free(ctx);
    return res;
  }

  int l = d_bytes_to(d_get(ctx->responses[0], K_RESULT), tx_hash, 32);
  ctx_free(ctx);
  return l;
}

static void verify_action_message(usn_device_conf_t* conf, d_token_t* msg, usn_msg_result_t* result) {
  bytes32_t  hash;
  address_t  sender;
  char       tmp[400], mhash[500];
  in3_ctx_t* ctx = NULL;
  result->device = find_device(conf, d_get_stringk(msg, K_URL));
  rejectp_if(!result->device, "the device with this url does not exist");

  // prepare message hash
  //TODO  the timestamp would run out space around 2106 !
  sprintf(tmp, "%s%u%s{}", result->device->url, d_get_intk(msg, K_TIMESTAMP), d_get_stringk(msg, K_ACTION));
  sprintf(mhash, "\031Ethereum Signed Message:\n%u%s", (unsigned int) strlen(tmp), tmp);
  bytes_t msg_data = {.data = (uint8_t*) mhash, .len = strlen(mhash)};
  sha3_to(&msg_data, hash);
  msg_data = bytes(hash, 32);

  // get the signature
  bytes_t* signer = ecrecover_signature(&msg_data, d_get(msg, K_SIGNATURE));
  if (signer && signer->len != 20) {
    b_free(signer);
    signer = NULL;
  }
  rejectp_if(!signer, "the message was not signed");
  memcpy(sender, signer->data, 20);
  b_free(signer);

  // look for a transaction hash
  bytes_t* tx_hash = d_get_bytesk(msg, K_TRANSACTIONHASH);
  rejectp_if(tx_hash && tx_hash->len != 32, "incorrect transactionhash");

  if (!tx_hash) {
    // without a tx_hash, we can only call "hasAccess()" of the contract.
    uint8_t   calldata[64];
    bytes32_t access;
    bytes_t   action_bytes = d_to_bytes(d_get(msg, K_ACTION));
    memset(calldata, 0, 64);
    memcpy(calldata + 12, signer->data, 20); // the signer
    sha3_to(&action_bytes, calldata + 32);   // add the hash of the action
    memset(calldata + 32 + 4, 0, 28);        // set the rest of the data to 0

    in3_ret_t l = exec_eth_call(conf, "a0b0305f", result->device->id, bytes(calldata, 64), access, 32);
    rejectp_if(l < 0, "The has_access could not be verified");
    rejectp_if(access + l - 1 == 0, "Access rejected");

  } else {

    // we keep the data of the last receipt, so we don't need to verify them again.
    static usn_booking_t last_receipt;
    usn_booking_t        r;

    // store the txhash
    memcpy(r.tx_hash, tx_hash->data, 32);

    // can we use a chached version?
    if (memcmp(last_receipt.tx_hash, tx_hash->data, 32) == 0) {
      // same hash, so we can copy the last one
      r = last_receipt;
    } else {
      // build request
      char params[71];
      strcpy(params, "[\"0x");
      for (int i = 0; i < 32; i++) sprintf(params + 4 + 2 * i, "%02x", tx_hash->data[i]);
      strcpy(params + 4 + 64, "\"]");

      // send the request
      ctx = in3_client_rpc_ctx(conf->c, "eth_getTransactionReceipt", params);

      dbg_log("params after sending %s\n", params);
      dbg_log("error for receipt: %s\n", ctx->error);

      // do we have a valid result?
      rejectp_if(ctx->error, "The transaction receipt could not be verified");
      rejectp_if(!ctx->responses || !ctx->responses[0] || !d_get(ctx->responses[0], K_RESULT), "No useable response found");

      // find the event
      d_token_t* event = get_rented_event(d_get(ctx->responses[0], K_RESULT));
      rejectp_if(!event || d_type(event) != T_OBJECT, "the tx receipt or the event could not be found");

      // extract the values
      bytes_t* data      = d_get_bytesk(event, K_DATA);
      bytes_t* address   = d_get_byteskl(event, K_ADDRESS, 20);
      bytes_t* device_id = d_bytesl(d_get_at(d_get(event, K_TOPICS), 2), 32);
      r.rented_from      = bytes_to_long(data->data + 32, 32);
      r.rented_until     = bytes_to_long(data->data + 64, 32);
      memcpy(r.controller, data->data + 12, 20);

      dbg_log("device_id in topic (len=%u) : 0x%02x%02x%02x%02x\n", device_id->len, device_id->data[0], device_id->data[1], device_id->data[2], device_id->data[3]);
      dbg_log("device_id in result  : 0x%02x%02x%02x%02x\n", result->device->id[0], result->device->id[1], result->device->id[2], result->device->id[3]);
      // check device_id and contract
      rejectp_if(!device_id || device_id->len != 32 || memcmp(device_id->data, result->device->id, 32), "Invalid DeviceId");
      rejectp_if(!address || address->len != 20 || memcmp(address->data, conf->contract, 20), "Invalid contract");

      // cache it for next time.
      last_receipt = r;
    }

    // check if the time and sender is correct
    uint64_t now = conf->now ? conf->now : d_get_longk(msg, K_TIMESTAMP);
    rejectp_if(r.rented_from >= r.rented_until || r.rented_from > now || r.rented_until < now, "Invalid Time");
    dbg_log("sender      : 0x%02x%02x%02x%02x\n", sender[0], sender[1], sender[2], sender[3]);
    dbg_log("controller  : 0x%02x%02x%02x%02x\n", r.controller[0], r.controller[1], r.controller[2], r.controller[3]);
    rejectp_if(memcmp(sender, r.controller, 20), "Invalid signer of the signature");
  }

  result->accepted = true;
  strcpy(result->action, d_get_stringk(msg, K_ACTION)); // this is not nice to overwrite the original payload, but this way we don't need to free it.

clean:
  if (ctx) ctx_free(ctx);
}

usn_msg_result_t usn_verify_message(usn_device_conf_t* conf, char* message) {

  // prepare message
  usn_msg_result_t result = {.accepted = false, .error_msg = NULL, .action = message, .id = 0};
  json_ctx_t*      parsed = parse_json(message);
  reject_if(!message, "no message passed");
  reject_if(!parsed, "error parsing the json-message");
  reject_if(!conf, "no config passed");
  reject_if(!conf->chain_id, "chain_id missing in config");

  // check message type
  char* msgType = d_get_stringk(parsed->result, K_MSG_TYPE);
  result.id     = d_get_intk(parsed->result, K_ID);
  reject_if(!parsed->result || d_type(parsed->result) != T_OBJECT, "no message-object passed");
  reject_if(!msgType || strlen(msgType) == 0, "the messageType is missing");

  // handle message type
  if (strcmp(msgType, "action") == 0) {
    result.msg_type = USN_ACTION;
    verify_action_message(conf, parsed->result, &result);
  } else if (strcmp(msgType, "in3Response") == 0) {
    result.msg_type = USN_RESPONSE;
    result.accepted = true;
  } else
    result.error_msg = "Unknown message type";
  json_free(parsed);

  return result;
}

in3_ret_t usn_register_device(usn_device_conf_t* conf, char* url) {
  usn_url_t parsed = usn_parse_url(url);
  if (!parsed.contract_name) return -1;
  if (conf->len_devices == 0)
    conf->devices = _malloc(sizeof(usn_device_t));
  else
    conf->devices = _realloc(conf->devices, sizeof(usn_device_t) * (conf->len_devices + 1), sizeof(usn_device_t) * conf->len_devices);
  if (conf->devices == NULL) return IN3_ENOMEM;
  usn_device_t* device    = conf->devices + conf->len_devices;
  device->url             = url;
  device->num_bookings    = 0;
  device->current_booking = -1;
  device->bookings        = NULL;
  memcpy(device->id, parsed.device_id, 32);
  conf->len_devices++;
  return IN3_OK;
}

usn_url_t usn_parse_url(char* url) {
  usn_url_t res;
  memset(&res, 0, sizeof(usn_url_t));
  res.contract_name = strchr(url, '@');
  if (!res.contract_name) return res;
  char* c = strchr(url, '#');
  if (c) {
    char         counter[20];
    unsigned int len = res.contract_name - c - 1;
    strncpy(counter, c + 1, len);
    counter[min((sizeof(counter) - 1), len)] = '\0';
    res.counter                              = atoi(counter);
  } else
    c = res.contract_name;
  bytes_t name = bytes((uint8_t*) url, c - url);
  sha3_to(&name, res.device_id);
  long_to_bytes(res.counter, res.device_id + 24);
  res.contract_name++;
  return res;
}

static int usn_add_booking(usn_device_t* device, address_t controller, uint64_t rented_from, uint64_t rented_until, uint8_t* props, bytes32_t tx_hash) {
  for (int i = 0; i < device->num_bookings; i++) {
    if (device->bookings[i].rented_from == rented_from) {
      device->bookings[i].rented_until = rented_until;
      if (props) memcpy(device->bookings[i].props, props, 16);
      return 0;
    }
  }
  device->bookings = device->bookings
                         ? _realloc(device->bookings, sizeof(usn_booking_t) * (device->num_bookings + 1), sizeof(usn_booking_t) * device->num_bookings)
                         : _malloc(sizeof(usn_booking_t) * device->num_bookings + 1);
  usn_booking_t* booking = device->bookings + device->num_bookings;
  booking->rented_from   = rented_from;
  booking->rented_until  = rented_until;
  memcpy(booking->controller, controller, 20);
  memcpy(booking->tx_hash, tx_hash, 32);
  if (props)
    memcpy(booking->props, props, 16);
  else
    memset(booking->props, 0, 16);
  device->num_bookings++;
  return 1;
}

in3_ret_t usn_update_bookings(usn_device_conf_t* conf) {
  // first we get the current BlockNumber
  in3_ctx_t* ctx = in3_client_rpc_ctx(conf->c, "eth_blockNumber", "[]");
  in3_ret_t  res = ctx_get_error(ctx, 0);
  if (res != IN3_OK) {
    ctx_free(ctx);
    return res;
  }
  uint64_t current_block = d_get_longk(ctx->responses[0], K_RESULT);
  ctx_free(ctx);
  if (conf->last_checked_block == current_block) return IN3_OK;

  if (!conf->last_checked_block) {
    // now we get all the current bookings
    uint8_t tmp[128]; // we need 128 byte to store one state
    for (int i = 0; i < conf->len_devices; i++) {
      usn_device_t* device = conf->devices + i;

      // get the number of bookings and manage memory
      if (0 > (res = exec_eth_call(conf, "0x3fce7fcf", device->id, bytes(NULL, 0), tmp, 32))) return res;
      if (device->bookings) _free(device->bookings);

#ifdef __clang_analyzer__
      // let the analyser know that this can not be garbage values
      memset(tmp, 0, 128);
#endif
      int size             = bytes_to_int(tmp + 28, 4);
      device->bookings     = size ? _calloc(sizeof(usn_booking_t), size) : NULL;
      device->num_bookings = 0;

      for (int n = 0; n < size; n++) {
        // create the input data ( the index )
        memset(tmp, 0, 32);
        int_to_bytes(n, tmp + 28);

        //  call the function in solidity: getState(bytes32 id, uint index) external view returns (address controller, uint64 rentedFrom, uint64 rentedUntil, uint128 properties);
        if (0 > (res = exec_eth_call(conf, "0x29dd2f8e", device->id, bytes(tmp, 32), tmp, 128))) return res; // call getState()
        usn_booking_t* booking = device->bookings + device->num_bookings;
        booking->rented_from   = bytes_to_long(tmp + 32 + 24, 8);
        booking->rented_until  = bytes_to_long(tmp + 64 + 24, 8);
        memcpy(booking->controller, tmp + 12, 20);
        memcpy(booking->props, tmp + 96 + 16, 16);
        if (!booking->rented_from) continue;
        device->num_bookings++;
      }
    }
  } else {
    // look for events
    // build request
    char *params = alloca(conf->len_devices * 70 + 320), *p = params + sprintf(params, "[{\"address\":\"0x");
    p += bytes_to_hex(conf->contract, 20, p);
    p += sprintf(p, "\", \"topics\":[[\"0x9123e6a7c5d144bd06140643c88de8e01adcbb24350190c02218a4435c7041f8\",\"0x63febe59689bc8e2235e549f5f941933c2ba8a6f470fa2db0badaab584c758b9\"],null,");
    if (conf->len_devices == 1) {
      p += sprintf(p, "\"0x");
      p += bytes_to_hex(conf->devices->id, 32, p);
      p += sprintf(p, "\"");
    } else {
      p += sprintf(p, "[");
      for (int k = 0; k < conf->len_devices; k++) {
        if (k) p += sprintf(p, ",");
        p += sprintf(p, "\"0x");
        p += bytes_to_hex(conf->devices[k].id, 32, p);
        p += sprintf(p, "\"");
      }
      p += sprintf(p, "]");
    }
    sprintf(p, "],\"fromBlock\":\"0x%" PRIx64 "\",\"toBlock\":\"0x%" PRIx64 "\"}]", conf->last_checked_block + 1, current_block);

    // send the request
    ctx = in3_client_rpc_ctx(conf->c, "eth_getLogs", params);

    // do we have a valid result?
    if ((res = ctx_get_error(ctx, 0))) {
      ctx_free(ctx);
      return res;
    }

    // let's iterate over the found events
    for (d_iterator_t iter = d_iter(d_get(ctx->responses[0], K_RESULT)); iter.left; d_iter_next(&iter)) {
      d_token_t*    topics = d_get(iter.token, K_TOPICS);
      bytes_t*      t0     = d_bytesl(d_get_at(topics, 0), 32);
      usn_device_t* device = find_device_by_id(conf, d_to_bytes(d_get_at(topics, 2)).data);
      bytes_t*      data   = d_get_bytesk(iter.token, K_DATA);
      if (t0->len != 32 || !device || !data) continue;
      usn_add_booking(device, data->data + 12,
                      bytes_to_long(data->data + 32 + 24, 8),
                      bytes_to_long(data->data + 64 + 24, 8),
                      *(t0->data) == 0x63 ? NULL : data->data + 6 + 32 + 16,
                      d_get_bytesk(iter.token, K_TRANSACTION_HASH)->data);
    }

    ctx_free(ctx);
  }

  // update the last_block
  conf->last_checked_block = current_block;

  return IN3_OK;
}

void usn_remove_old_bookings(usn_device_conf_t* conf) {
  for (int i = 0; i < conf->len_devices; i++) {
    usn_device_t* device = conf->devices + i;
    for (int n = 0; n < device->num_bookings; n++) {
      usn_booking_t* b = device->bookings + n;
      if (b->rented_until <= conf->now) {
        // remove it
        if (n + 1 < device->num_bookings) memmove(b, b + 1, sizeof(usn_booking_t) * (device->num_bookings - n - 1));
        device->num_bookings--;
        n--;
      }
    }
  }
}

usn_event_t usn_get_next_event(usn_device_conf_t* conf) {
  usn_event_t e = {.ts = 0xFFFFFFFFFFFFFFFF, .device = NULL, .type = BOOKING_NONE};
  for (int i = 0; i < conf->len_devices; i++) {
    usn_device_t* device = conf->devices + i;
    for (int n = 0; n < device->num_bookings; n++) {
      usn_booking_t* b = device->bookings + n;
      if (b->rented_until < e.ts) {
        e.type   = BOOKING_STOP;
        e.ts     = b->rented_until;
        e.device = device;
      }
      if ((b->rented_from > conf->now || (device->current_booking == -1 && b->rented_until > conf->now)) && b->rented_from < e.ts) {
        e.type   = BOOKING_START;
        e.ts     = b->rented_from;
        e.device = device;
      }
    }
  }

  return e;
}

static int usn_event_handled(usn_event_t* event) {
  usn_device_t* device = event->device;
  for (int n = 0; n < device->num_bookings; n++) {
    usn_booking_t* b = device->bookings + n;
    if (event->type == BOOKING_START && b->rented_from == event->ts) {
      device->current_booking = n;
      return 0;
    }
    if (event->type == BOOKING_STOP && b->rented_until == event->ts) {
      usn_booking_t* b = device->bookings + n;
      // remove it
      if (n + 1 < device->num_bookings) memmove(b, b + 1, sizeof(usn_booking_t) * (device->num_bookings - n - 1));
      device->num_bookings--;
      event->device->current_booking = -1;
      return 0;
    }
  }
  return -1;
}

static uint64_t check_actions(usn_device_conf_t* conf) {
  usn_event_t e;
  while (true) {
    e = usn_get_next_event(conf);
    // we need to execute this now!
    if (e.type != BOOKING_NONE && e.ts <= conf->now && e.device) {
      if (conf->booking_handler)
        conf->booking_handler(&e);
      usn_event_handled(&e);
    } else
      return e.ts;
  }
}

unsigned int usn_update_state(usn_device_conf_t* conf, unsigned int wait_time) {
  // check if there is anything to do now
  check_actions(conf);

  // update all bookings
  usn_update_bookings(conf);

  // check again, if there was a new event we should handle now.
  uint64_t next_action = check_actions(conf);

  // how long show we sleep before checking again?
  return next_action - conf->now < wait_time ? next_action - conf->now : wait_time;
}

in3_ret_t usn_price(in3_t* c, address_t contract, address_t token, char* url, uint32_t seconds, address_t controller, bytes32_t price) {
  usn_device_conf_t conf = {.c = c};
  memcpy(conf.contract, contract, 20);
  usn_url_t purl = usn_parse_url(url);
  uint8_t   params[96];
  memset(params, 0, 96);
  if (controller) memcpy(params + 12, controller, 20);
  int_to_bytes(seconds, params + 60);
  if (token) memcpy(params + 64 + 12, token, 20);
  return exec_eth_call(&conf, "0xf44fb0a4", purl.device_id, bytes(NULL, 0), price, 32) < 0 ? IN3_EINVALDT : IN3_OK;
  //     function price(bytes32 id, address user, uint32 secondsToRent, address token) public constant returns (uint128);
}

in3_ret_t usn_rent(in3_t* c, address_t contract, address_t token, char* url, uint32_t seconds, bytes32_t tx_hash) {
  usn_device_conf_t conf = {.c = c};
  memcpy(conf.contract, contract, 20);
  usn_url_t purl = usn_parse_url(url);
  uint8_t   params[100];
  memset(params, 0, 100);

  int_to_bytes(seconds, params + 60);
  if (token) memcpy(params + 64 + 12, token, 20);

  // first get the price
  bytes32_t price;
  in3_ret_t res = exec_eth_call(&conf, "0xf44fb0a4", purl.device_id, bytes(params, 96), price, 32);
  if (res < 0) return res;

  // now send the tx
  memset(params, 0, 100);
  hex_to_bytes("400a6315", -1, params, 4); //  function rent(bytes32 id, uint32 secondsToRent, address token) external payable;
  memcpy(params + 4, purl.device_id, 32);
  int_to_bytes(seconds, params + 64);
  if (token) memcpy(params + 80, token, 20);
#ifdef __clang_analyzer__
  // let the analyser know that this can not be garbage values
  memset(price, 0, 32);
#endif

  res = exec_eth_send(&conf, bytes(params, 100), price, tx_hash);
  if (res < 0) return res;
  return IN3_OK;
}

in3_ret_t usn_return(in3_t* c, address_t contract, char* url, bytes32_t tx_hash) {
  usn_device_conf_t conf = {.c = c};
  memcpy(conf.contract, contract, 20);
  usn_url_t purl       = usn_parse_url(url);
  uint8_t   params[36] = {0};

  // now send the tx
  hex_to_bytes("896e4b2c", -1, params, 4); //  function rent(bytes32 id, uint32 secondsToRent, address token) external payable;
  memcpy(params + 4, purl.device_id, 32);

  in3_ret_t res = exec_eth_send(&conf, bytes(params, 100), NULL, tx_hash);
  if (res < 0) return res;
  return IN3_OK;
}
