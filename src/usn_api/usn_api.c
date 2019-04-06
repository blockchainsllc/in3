#include "./usn_api.h"
#include "../core/client/context.h"
#include "../core/client/keys.h"
#include "../eth_nano/eth_nano.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define K_MSG_TYPE key("msgType")
#define K_URL key("url")
#define K_ACTION key("action")
#define K_TRANSACTIONHASH key("transactionHash")
#define K_SIGNATURE key("signature")

#define reject_if(c, m)            \
  if (c) {                         \
    if (parsed) free_json(parsed); \
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

//static int exec_eth_call(in3_t* c, address_t to, )

static d_token_t* get_rented_event(d_token_t* receipt) {
  bytes32_t event_hash;
  hex2byte_arr("9123e6a7c5d144bd06140643c88de8e01adcbb24350190c02218a4435c7041f8", 64, event_hash, 32);
  for (d_iterator_t iter = d_iter(d_get(receipt, K_LOGS)); iter.left; d_iter_next(&iter)) {
    bytes_t* t = d_get_bytes_at(d_get(iter.token, K_TOPICS), 0);
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

static int exec_eth_call(usn_device_conf_t* conf, char* fn_hash, bytes32_t device_id, bytes_t data, uint8_t* result) {
  int     l = 4 + 32 + data.len;
  uint8_t cdata[l];
  hex2byte_arr(fn_hash, -1, cdata, 4);
  memcpy(cdata + 4, device_id, 32);
  if (data.len) memcpy(cdata + 36, data.data, data.len);

  char *args[l + l + 100], *p = (char*) args + sprintf((char*) args, "[{\"data\":\"0x");
  for (int i = 0; i < 100; i++) p += sprintf(p, "%02x", cdata[i]);
  p += sprintf(p, "\",\"gas\":\"0x77c810\",\"to\":\"0x");
  for (int i = 0; i < 20; i++) p += sprintf(p, "%02x", conf->contract[i]);
  p += sprintf(p, "\"},\"latest\"]");

  // send the request
  in3_ctx_t* ctx = in3_client_rpc_ctx(conf->c, "eth_call", (char*) args);

  // do we have a valid result?
  if (ctx->error || !ctx->responses || !ctx->responses[0] || !d_get(ctx->responses[0], K_RESULT)) {
    free_ctx(ctx);
    return -1;
  }
  free_ctx(ctx);
  return d_bytes_to(d_get(ctx->responses[0], K_RESULT), result, 0xFFFFFFFF);
}

static void verify_action_message(usn_device_conf_t* conf, d_token_t* msg, usn_msg_result_t* result) {
  bytes32_t  hash;
  address_t  sender;
  char       tmp[400], mhash[500];
  in3_ctx_t* ctx = NULL;
  result->device = find_device(conf, d_get_stringk(msg, K_URL));
  rejectp_if(!result->device, "the device with this url does not exist");

  // prepare message hash
  sprintf(tmp, "%s%" PRIu64 "%s{}", result->device->url, d_get_longk(msg, K_TIMESTAMP), d_get_stringk(msg, K_ACTION));
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

    int l = exec_eth_call(conf, "a0b0305f", result->device->id, bytes(calldata, 64), access);
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

      // do we have a valid result?
      rejectp_if(ctx->error, "The transaction receipt could not be verified");
      rejectp_if(!ctx->responses || !ctx->responses[0] || !d_get(ctx->responses[0], K_RESULT), "No useable response found");

      // find the event
      d_token_t* event = get_rented_event(d_get(ctx->responses[0], K_RESULT));
      rejectp_if(!event || d_type(event) != T_OBJECT, "the tx receipt or the event could not be found");

      // extract the values
      bytes_t* data      = d_get_bytesk(event, K_DATA);
      bytes_t* address   = d_get_bytesk(event, K_ADDRESS);
      bytes_t* device_id = d_get_bytes_at(d_get(event, K_TOPICS), 1);
      r.rented_from      = bytes_to_long(data->data + 32, 32);
      r.rented_until     = bytes_to_long(data->data + 64, 32);
      r.controller       = data->data + 12;

      // check device_id and contract
      rejectp_if(!device_id || device_id->len != 32 || memcmp(device_id->data, result->device->id, 32), "Invalid DeviceId");
      rejectp_if(!address || address->len != 20 || memcmp(address->data, conf->contract, 20), "Invalid contract");

      // cache it for next time.
      last_receipt = r;
    }

    // check if the time and sender is correct
    uint64_t now = conf->now ? conf->now : d_get_longk(msg, K_TIMESTAMP);
    rejectp_if(r.rented_from >= r.rented_until || r.rented_from > now || r.rented_until < now, "Invalid Time");
    rejectp_if(memcmp(sender, r.controller, 20), "Invalid signer of the signature");
  }

  result->accepted = true;
  strcpy(result->action, d_get_stringk(msg, K_ACTION)); // this is not nice to overwrite the original payload, but this way we don't need to free it.

clean:
  if (ctx) free_ctx(ctx);
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
  free_json(parsed);

  return result;
}

int usn_register_device(usn_device_conf_t* conf, char* url) {
  usn_url_t parsed = usn_parse_url(url);
  if (!parsed.contract_name) return -1;
  if (conf->len_devices == 0)
    conf->devices = _malloc(sizeof(usn_device_t));
  else
    conf->devices = _realloc(conf->devices, sizeof(usn_device_t) * (conf->len_devices + 1), sizeof(usn_device_t) * conf->len_devices);
  if (conf->devices == NULL) return -1;
  usn_device_t* device = conf->devices + conf->len_devices;
  device->url          = url;
  device->num_bookings = 0;
  device->bookings     = NULL;
  memcpy(device->id, parsed.device_id, 32);
  conf->len_devices++;

  return 0;
}

usn_url_t usn_parse_url(char* url) {
  usn_url_t res;
  memset(&res, 0, sizeof(usn_url_t));
  res.contract_name = strchr(url, '@');
  if (!res.contract_name) return res;
  char* c = strchr(url, '#');
  if (c) {
    char counter[20];
    strncpy(counter, c + 1, res.contract_name - c - 1);
    res.counter = atoi(counter);
  } else
    c = res.contract_name;
  bytes_t name = bytes((uint8_t*) url, c - url);
  sha3_to(&name, res.device_id);
  long_to_bytes(res.counter, res.device_id + 24);
  res.contract_name++;
  return res;
}

int usn_add_booking(usn_device_t* device, address_t controller, uint64_t rented_from, uint64_t rented_until, uint8_t* props, bytes32_t tx_hash) {
  for (int i = 0; i < device->num_bookings; i++) {
    if (device->bookings[i].rented_from == rented_from) {
      device->bookings[i].rented_until = rented_until;
      memcpy(device->bookings[i].props, props, 16);
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
  memcpy(booking->props, props, 16);
  memcpy(booking->tx_hash, tx_hash, 32);
  device->num_bookings++;
  return 1;
}

int usn_update_bookings(usn_device_conf_t* conf) {
  // first we get the current BlockNumber
  in3_ctx_t* ctx = in3_client_rpc_ctx(conf->c, "eth_blockNumber", "[]");
  if (ctx->error || !ctx->responses || !ctx->responses[0] || !d_get(ctx->responses[0], K_RESULT)) {
    free_ctx(ctx);
    return -1;
  }
  uint64_t current_block = d_get_longk(ctx->responses[0], K_RESULT);
  free_ctx(ctx);
  if (conf->last_checked_block == current_block) return 0;

  if (!conf->last_checked_block) {
    // now we get all the current bookings
    uint8_t tmp[128]; // we need 128 byte to store one state
    for (int i = 0; i < conf->len_devices; i++) {
      usn_device_t* device = conf->devices + i;

      // get the number of bookings and manage memory
      if (exec_eth_call(conf, "0x3fce7fcf", device->id, bytes(NULL, 0), tmp) != 32) return -1;
      if (device->bookings) _free(device->bookings);
      device->num_bookings = bytes_to_int(tmp + 28, 4);
      device->bookings     = device->num_bookings ? _calloc(sizeof(usn_booking_t), device->num_bookings) : NULL;

      for (int n = 0; n < device->num_bookings; n++) {
        // create the input data ( the index )
        memset(tmp, 0, 32);
        int_to_bytes(n, tmp + 28);

        //  call the function in solidity: getState(bytes32 id, uint index) external view returns (address controller, uint64 rentedFrom, uint64 rentedUntil, uint128 properties);
        if (exec_eth_call(conf, "0x29dd2f8e", device->id, bytes(tmp, 32), tmp) != 128) return -1; // call getState()
        usn_booking_t* booking = device->bookings + n;
        booking->rented_from   = bytes_to_long(tmp + 32 + 24, 8);
        booking->rented_until  = bytes_to_long(tmp + 64 + 24, 8);
        memcpy(booking->controller, tmp + 12, 20);
        memcpy(booking->props, tmp + 96 + 16, 16);
      }
    }
  } else {
    // look for events
    // build request
    char  params[conf->len_devices * 70 + 320];
    char* p = params + sprintf(params, "[{\"address\":\"0x");
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
    p += sprintf(p, "],\"fromBlock\":\"0x%" PRIx64 "\",\"toBlock\":\"0x%" PRIx64 "\"]}]", conf->last_checked_block + 1, current_block);

    // send the request
    ctx = in3_client_rpc_ctx(conf->c, "eth_getLogs", params);

    // do we have a valid result?
    if (ctx->error || !ctx->responses || !ctx->responses[0] || !d_get(ctx->responses[0], K_RESULT)) {
      free_ctx(ctx);
      return -1;
    }

    // let's iterate over the found events
    for (d_iterator_t iter = d_iter(d_get(ctx->responses[0], K_RESULT)); iter.left; d_iter_next(&iter)) {
      d_token_t*    topics = d_get(iter.token, K_TOPICS);
      bytes_t       t0     = d_to_bytes(d_get_at(topics, 0));
      usn_device_t* device = find_device_by_id(conf, d_to_bytes(d_get_at(topics, 2)).data);
      bytes_t*      data   = d_get_bytesk(iter.token, K_DATA);
      if (t0.len != 32 || !device || !data) continue;
      usn_add_booking(device, data->data + 12,
                      bytes_to_long(data->data + 32 + 24, 8),
                      bytes_to_long(data->data + 64 + 24, 8),
                      *t0.data == 0x63 ? NULL : data->data + 6 + 32 + 16,
                      d_get_bytesk(iter.token, K_TRANSACTION_HASH)->data);
    }

    free_ctx(ctx);
  }

  // update the last_block
  conf->last_checked_block = current_block;

  return 0;
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
      if (b->rented_from <= conf->now && b->rented_from < e.ts) {
        e.type   = BOOKING_STOP;
        e.ts     = b->rented_until;
        e.device = device;
      } else if (b->rented_from > conf->now && b->rented_from < e.ts) {
        e.type   = BOOKING_START;
        e.ts     = b->rented_from;
        e.device = device;
      }
    }
  }
  return e;
}