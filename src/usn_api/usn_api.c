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

#define check_or_return(c, m)      \
  if (c) {                         \
    if (parsed) free_json(parsed); \
    result.error_msg = m;          \
    result.action    = NULL;       \
    return result;                 \
  }
#define checkp_or_return(c, m) \
  if (c) {                     \
    result->error_msg = m;     \
    result->action    = NULL;  \
    goto clean;                \
  }

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

static void verify_action_message(in3_t* c, d_token_t* msg, usn_device_conf_t* conf, usn_msg_result_t* result) {
  bytes32_t  hash;
  address_t  sender;
  char       tmp[400], mhash[500];
  in3_ctx_t* ctx = NULL;
  result->device = find_device(conf, d_get_stringk(msg, K_URL));
  checkp_or_return(!result->device, "the device with this url does not exist");

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
  checkp_or_return(!signer, "the message was not signed");
  memcpy(sender, signer->data, 20);
  b_free(signer);

  // look for a transaction hash
  bytes_t* tx_hash = d_get_bytesk(msg, K_TRANSACTIONHASH);
  checkp_or_return(tx_hash && tx_hash->len != 32, "incorrect transactionhash");

  if (!tx_hash) {
    // without a tx_hash, we can only call "hasAccess()" of the contract.
    uint8_t calldata[100];
    bytes_t action_bytes = d_to_bytes(d_get(msg, K_ACTION));
    memset(calldata, 0, 100);
    hex2byte_arr("a0b0305f", 8, calldata, 4);         // functionhash for canAccess()
    memcpy(calldata + 4, result->device->id, 32);     // the device_id
    memcpy(calldata + 4 + 32 + 12, signer->data, 20); // the signer
    sha3_to(&action_bytes, calldata + 4 + 32 + 32);   // add the hash of the action
    memset(calldata + 4 + 32 + 32 + 4, 0, 28);        // set the rest of the data to 0

    // build request
    char *args[300], *p = (char*) args + sprintf((char*) args, "[{\"data\":\"0x");
    for (int i = 0; i < 100; i++) p += sprintf(p, "%02x", calldata[i]);
    p += sprintf(p, "\",\"gas\":\"0x77c810\",\"to\":\"0x");
    for (int i = 0; i < 20; i++) p += sprintf(p, "%02x", conf->contract[i]);
    p += sprintf(p, "\"},\"latest\"]");

    // send the request
    ctx = in3_client_rpc_ctx(c, "eth_call", (char*) args);

    // do we have a valid result?
    checkp_or_return(ctx->error, "The transaction receipt could not be verified");
    checkp_or_return(!ctx->responses || !ctx->responses[0] || !d_get(ctx->responses[0], K_RESULT), "No useable response found");
    checkp_or_return(d_get_intk(ctx->responses[0], K_RESULT) != 1, "Access rejected");

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
      ctx = in3_client_rpc_ctx(c, "eth_getTransactionReceipt", params);

      // do we have a valid result?
      checkp_or_return(ctx->error, "The transaction receipt could not be verified");
      checkp_or_return(!ctx->responses || !ctx->responses[0] || !d_get(ctx->responses[0], K_RESULT), "No useable response found");

      // find the event
      d_token_t* event = get_rented_event(d_get(ctx->responses[0], K_RESULT));
      checkp_or_return(!event || d_type(event) != T_OBJECT, "the tx receipt or the event could not be found");

      // extract the values
      bytes_t* data      = d_get_bytesk(event, K_DATA);
      bytes_t* address   = d_get_bytesk(event, K_ADDRESS);
      bytes_t* device_id = d_get_bytes_at(d_get(event, K_TOPICS), 1);
      r.rented_from      = bytes_to_long(data->data + 32, 32);
      r.rented_until     = bytes_to_long(data->data + 64, 32);
      r.controller       = data->data + 12;

      // check device_id and contract
      checkp_or_return(!device_id || device_id->len != 32 || memcmp(device_id->data, result->device->id, 32), "Invalid DeviceId");
      checkp_or_return(!address || address->len != 20 || memcmp(address->data, conf->contract, 20), "Invalid contract");

      // cache it for next time.
      last_receipt = r;
    }

    // check if the time and sender is correct
    uint64_t now = conf->now ? conf->now : d_get_longk(msg, K_TIMESTAMP);
    checkp_or_return(r.rented_from >= r.rented_until || r.rented_from > now || r.rented_until < now, "Invalid Time");
    checkp_or_return(memcmp(sender, r.controller, 20), "Invalid signer of the signature");
  }

  result->accepted = true;
  strcpy(result->action, d_get_stringk(msg, K_ACTION)); // this is not nice to overwrite the original payload, but this way we don't need to free it.

clean:
  if (ctx) free_ctx(ctx);
}

usn_msg_result_t usn_verify_message(in3_t* c, char* message, usn_device_conf_t* conf) {

  // prepare message
  usn_msg_result_t result = {.accepted = false, .error_msg = NULL, .action = message, .id = 0};
  json_ctx_t*      parsed = parse_json(message);
  check_or_return(!message, "no message passed");
  check_or_return(!parsed, "error parsing the json-message");
  check_or_return(!conf, "no config passed");
  check_or_return(!conf->chain_id, "chain_id missing in config");

  // check message type
  char* msgType = d_get_stringk(parsed->result, K_MSG_TYPE);
  result.id     = d_get_intk(parsed->result, K_ID);
  check_or_return(!parsed->result || d_type(parsed->result) != T_OBJECT, "no message-object passed");
  check_or_return(!msgType || strlen(msgType) == 0, "the messageType is missing");

  // handle message type
  if (strcmp(msgType, "action") == 0) {
    result.msg_type = USN_ACTION;
    verify_action_message(c, parsed->result, conf, &result);
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
  conf->devices[conf->len_devices].url = url;
  memcpy(conf->devices[conf->len_devices].id, parsed.device_id, 32);
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