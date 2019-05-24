#include "fsm.h"
#include "util/debug.h"
#include "util/stringbuilder.h"
#include <client/client.h>
#include <crypto/ecdsa.h>
#include <crypto/secp256k1.h>
#include <eth_nano.h>
#include <rtc.h>
#include <stdio.h>
#include <string.h>
#include <util/data.h>

msg_type_t msg_get_type(char* c) {
  char buf[20]; // allocate tmp space for string compare

  msg_type_t type = T_ERROR;
  if (strstr(c, "msgType") == 0) // if received string does not contain msgType
    return type;

  // NOTE: check buf size (will need an additional function like json_get_str_length
  //       or to modify json_get_str_value with additional parameter maxlen)

  json_get_str_value(c, "msgType", buf);
  if (strcmp(buf, "action") == 0) // if string match
    type = T_ACTION;
  else if (strcmp(buf, "in3Response") == 0) // if string match
    type = T_RESPONSE;

  return type;
}

action_type_t msg_get_action(char* c) {
  char buf[20]; // allocate tmp space for string compare

  action_type_t type = NONE;
  if (strstr(c, "action") == 0) // if received string does not contain action
    return type;

  // NOTE: check buf size (will need an additional function like json_get_str_length
  //       or to modify json_get_str_value with additional parameter maxlen)

  json_get_str_value(c, "action", buf); // note: check buf size (need some json_get_str_length)
  if (strcmp(buf, "unlock") == 0)       // if string match
    type = UNLOCK;
  else if (strcmp(buf, "lock") == 0) // if string match
    type = LOCK;

  return type;
}

char* msg_get_response(struct in3_client* c) {
  char* val = 0;

  if (!c->msg->ready)
    return T_ERROR;

  val = json_get_json_value(c->msg->data, "responses");
  return val;
}

// get_tx_hash - called by verify_rent (rent.c)
static int get_tx_hash(char* msg, char* dst) {
  *dst = 0;                                        // preset out string as an empty string
  json_get_str_value(msg, "transactionHash", dst); // note: check dst buf len
  if (strncmp(dst, "0x", 2))                       // EFnote: need to check also 0X ???
    return -1;                                     // good
  return 0;                                        // bad
}

// wait_for_message - called by send_ble
int wait_for_message(struct in3_client* c) {
  int i = 100;

  printk("<--- wait_for_message\n");
  while (c->msg->ready == 0 && i--)
    k_sleep(100);

  k_mutex_lock(&c->mutex, 30000);
  k_mutex_unlock(&c->mutex);

  if (c->msg->ready) {
    printk("<--- msg received!\n");
    return 0;
  }
  printk("<--- timeout error!\n");
  return i; // -1 if timeout elapsed
}

static struct in3_client* _client = NULL;

int send_ble(char** urls, int urls_len, char* pl, in3_response_t* result) {
  char payload[5120]; // EFmod -so big ?

  sprintf(payload, "{\"msgType\":\"in3Request\",\"msgId\":1,\"url\":\"%s\",\"method\":\"POST\",\"data\":%s}", *urls, pl);

  clear_message(_client);
  bluetooth_write_req(payload);
  int err = wait_for_message(_client);
  bluetooth_clear_req();

  _client->txr->data = NULL;

  if (err < 0) {
    sb_add_chars(&result->error, "Error receiving this response");
    return err;
  }

  _client->txr->data = msg_get_response(_client);
  sb_add_chars(&result->result, _client->txr->data);
  return 0;
}

// in3_get_tx_receipt - called by verify_rent;
int in3_get_tx_receipt(struct in3_client* c, char* tx_hash, char** response) {
  char  params[512]; // EFmod: (IN3_REQ_FMT string is 240 chars long)
  char* result = NULL;
  char* error  = NULL;

  if (!c || !tx_hash)
    return -1;

  _client           = c;
  c->in3->transport = send_ble;
  // first check if cached receipt matches
  if (c->txr->hash && !strcmp(c->txr->hash, tx_hash)) {
    //clear_message(c);
    *response = c->txr->data;
    return 0;
  }

  k_free(c->txr->data);
  k_free(c->txr->hash);
  memset(c->txr, 0, sizeof(in3_tx_receipt_t));

  sprintf(params, "[\"%s\"]", tx_hash);

  c->in3->transport = send_ble; // Assign the transport function (executed when needed)

  in3_client_rpc(c->in3, "eth_getTransactionReceipt", params, &result, &error);

  if (error) {
    printk("<--- got s error: %s\n", error);
    *response = NULL;
    k_free(error);
  } else {
    c->txr->data = result; // 190131 solves the issue in caching
    *response    = result;
    //TODO don't need it!
    c->txr->hash = k_calloc(1, strlen(tx_hash) + 1);
    c->txr->hash = memcpy(c->txr->hash, tx_hash, strlen(tx_hash));
  }

  return 0;
}

int in3_can_rent(struct in3_client* c, char* resp, char* amsg) {
  int            res = -1, i;
  char           tmp[256], mhash[256];
  json_parsed_t* response = parse_json(resp);
  json_parsed_t* message  = parse_json(amsg);
  d_token_t *    l, *log = NULL;
  bytes_t *      signer = NULL, *hash = NULL;
  bytes_t*       log_rented = hex2byte_new_bytes("9123e6a7c5d144bd06140643c88de8e01adcbb24350190c02218a4435c7041f8", 64);

  if (!response || !message) goto out;

  d_token_t* logs = d_get(response->items, key("logs"));
  if (!logs) goto out;

  for (i = 0, l = logs + 1; i < d_len(logs); i++, l = d_next(l)) {
    if (b_cmp(d_get_bytes_at(d_get(l, key("topics")), 0), log_rented)) {
      log = l;
      break;
    }
  }

  // no event found,
  if (!log) goto out;

  char* url = d_get_string(message->items, "url");
  //bytes_t* deviceId = d_get_bytes_at(d_get(log,key("topics")),1);  // we need to store and compare the deviceId
  bytes_t* data = d_get_bytes(log, "data");

  c->rent->from  = bytes_to_long(data->data + 32, 32);
  c->rent->until = bytes_to_long(data->data + 64, 32);
  //TODO do we need to set it as string?
  c->rent->controller = _malloc(43);
  if (c->rent->controller == NULL) goto out;
  c->rent->controller[0] = '0';
  c->rent->controller[1] = 'x';
  bytes_to_hex(data->data + 12, 20, c->rent->controller + 2);

  printk("*** controller: '%s'\n", c->rent->controller);
  printk("*** from: %d, until: %d, when: %d\n", c->rent->from, c->rent->until, c->rent->when);

  // wrong time
  if (c->rent->from >= c->rent->when || c->rent->when >= c->rent->until) goto out;

  // check signature

  // prepare message hash
  sprintf(tmp, "%s%d%s{}", url, c->rent->when, d_get_string(message->items, "action"));
  sprintf(mhash, "\031Ethereum Signed Message:\n%d%s", strlen(tmp), tmp);
  bytes_t msg = {.data = (uint8_t*) &mhash, .len = strlen(mhash)};
  hash        = sha3(&msg);

  // get the signature
  signer = ecrecover_signature(hash, d_get(message->items, key("signature")));
  if (signer == NULL) goto out;

  // check if the signer is the same as the controller in the event.
  // reuse msg to point to the address in the event.
  msg.data = data->data + 12;
  msg.len  = 20;
  if (!b_cmp(signer, &msg)) goto out;

  //TODO check if the url and the deviceid is correct.
  // if (strcmp(url, saved_url)!=0)          goto out;
  // if (!b_comp(device_id, saved_device_id)) goto out;
  // if (!b_comp( d_get_bytes(response,"contractAddress") , saved_contract)) goto out;

  res = 0;

out:
  if (response) _free(response);
  if (message) _free(message);
  if (hash) b_free(hash);
  if (signer) b_free(signer);
  b_free(log_rented);

  return res;
}

// verify_rent - called by in3_action (state machine, fsm.c)
int verify_rent(struct in3_client* c) {
  int   ret = -1, id    = 0;
  char  tx_hash[67], *r = 0;
  char* amsg = 0;
  char  payload[512]; // EFnote: the payload in send_ble is 10 times bigger

  get_tx_hash(c->msg->data, tx_hash); // get tx_hash from msg->data
  if (tx_hash[0] == 0)                // if empty string (errors in get_tx_hash)
    goto out;                         // note: ret = -1

  // keep copy of action message
  amsg = k_calloc(1, sizeof(char) * (c->msg->size + 1));
  amsg = memcpy(amsg, c->msg->data, c->msg->size + 1);
  id   = json_get_int_value(amsg, "msgId");

  in3_get_tx_receipt(c, tx_hash, &r); // try to receive response
  if (r)
    printk("<--- response:\n%s\n", r);

  if (in3_can_rent(c, r, amsg) < 0)
    goto out;

  ret = 0;

out:
  if (ret)
    sprintf(payload, "{\"msgId\":%d,\"msgType\":\"error\",\"error\":\"Invalid rental\"}", id);
  else
    sprintf(payload, "{\"msgId\":%d,\"msgType\":\"action\",\"result\":\"success\"}", id);

  bluetooth_write_req(payload);

  if (amsg)
    k_free(amsg);

  return ret;
}
