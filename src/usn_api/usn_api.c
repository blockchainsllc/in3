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

typedef struct {
  bytes32_t tx_hash;
  uint64_t  rented_from;
  uint64_t  rented_until;
  uint8_t*  controller;
} receipt_data_t;

static void verify_action_message(in3_t* c, d_token_t* msg, usn_device_conf_t* conf, usn_msg_result_t* result) {
  in3_ctx_t*            ctx = NULL;
  bytes32_t             hash;
  static receipt_data_t last_receipt;
  receipt_data_t        r;

  char tmp[400], mhash[500], *url = d_get_stringk(msg, K_URL);
  checkp_or_return(!url || strlen(url) == 0, "the url is missing");
  checkp_or_return(strcmp(conf->device_url, url), "wrong url");

  bytes_t* tx_hash = d_get_bytesk(msg, K_TRANSACTIONHASH);
  r.tx_hash        = tx_hash->data;
  checkp_or_return(!tx_hash || tx_hash->len != 32, "wrong or missing transactionHash");

  if (memcmp(last_receipt.tx_hash, tx_hash->data, 32) == 0) {
    // same hash, so we can copy the last one
    r = last_receipt;
  } else {
    // build request
    char params[71];
    strcpy(params, "[\"0x");
    for (int i = 0; i < 32; i++) sprintf(params + 4 + 2 * i, "%02x", tx_hash->data[i]);
    strcpy(params + 4 + 64, "\"]");
    ctx = in3_client_rpc_ctx(c, "eth_getTransactionReceipt", params);

    checkp_or_return(ctx->error, "The transaction receipt could not be verified");
    checkp_or_return(!ctx->responses || !ctx->responses[0] || !d_get(ctx->responses[0], K_RESULT), "No useable response found");

    // find the event
    d_token_t* event = get_rented_event(d_get(ctx->responses[0], K_RESULT));
    checkp_or_return(!event || d_type(event) != T_OBJECT, "the tx receipt or the event could not be found");

    // extract the values
    bytes_t* data      = d_get_bytesk(event, K_DATA);
    bytes_t* device_id = d_get_bytes_at(d_get(event, K_TOPICS), 1);
    r.rented_from      = bytes_to_long(data->data + 32, 32);
    r.rented_until     = bytes_to_long(data->data + 64, 32);
    r.controller       = data->data + 12;
    uint64_t now       = conf->now ? conf->now : d_get_longk(msg, K_TIMESTAMP);
  }

  // prepare message hash
  sprintf(tmp, "%s%" PRIu64 "%s{}", url, now, d_get_stringk(msg, K_ACTION));
  sprintf(mhash, "\031Ethereum Signed Message:\n%u%s", (unsigned int) strlen(tmp), tmp);
  bytes_t msg_data = {.data = (uint8_t*) mhash, .len = strlen(mhash)};
  sha3_to(&msg_data, hash);
  msg_data = bytes(hash, 32);

  // get the signature
  bytes_t* signer = ecrecover_signature(&msg_data, d_get(msg, K_SIGNATURE));
  checkp_or_return(!signer, "the message was not signed");
  bool valid_signer = signer->len == 20 && memcmp(signer->data, controller, 20) == 0;
  b_free(signer);
  checkp_or_return(!valid_signer, "invalid signature");
  checkp_or_return(!device_id || device_id->len != 32 || memcmp(device_id->data, conf->device_id, 32), "Invalid DeviceId");
  checkp_or_return(rented_from >= rented_until || rented_from > now || rented_until < now, "Invalid Time");

  result->accepted = true;
  strcpy(result->action, d_get_stringk(msg, K_ACTION)); // this is not nice to overwrite the original payload, but this way we don't need to free it.

clean:
  if (ctx) free_ctx(ctx);
}

usn_msg_result_t usn_verify_message(in3_t* c, char* message, usn_device_conf_t* conf) {
  usn_msg_result_t result = {.accepted = false, .error_msg = NULL, .action = message, .id = 0};
  json_ctx_t*      parsed = parse_json(message);
  check_or_return(!message, "no message passed");
  check_or_return(!parsed, "error parsing the json-message");
  char* msgType = d_get_stringk(parsed->result, K_MSG_TYPE);
  result.id     = d_get_intk(parsed->result, K_ID);
  check_or_return(!conf, "no config passed");
  check_or_return(!conf->chain_id, "chain_id missing in config");
  check_or_return(!conf->device_url, "url missing in config");
  check_or_return(!parsed->result || d_type(parsed->result) != T_OBJECT, "no message-object passed");
  check_or_return(!msgType || strlen(msgType) == 0, "the messageType is missing");
  if (strcmp(msgType, "action") == 0) {
    result.msg_type = USN_ACTION;
    verify_action_message(c, parsed->result, conf, &result);
  } else if (strcmp(msgType, "in3Response") == 0) {
    result.msg_type = USN_RESPONSE;
  } else
    result.error_msg = "Unknown message type";
  free_json(parsed);

  return result;
}
