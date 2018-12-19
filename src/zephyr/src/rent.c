#include <stdio.h>
#include <string.h>
#include <rtc.h>
#include <client/client.h>
#include "fsm.h"
#include "jsmn/jsmnutil.h"
#include "util/debug.h"
#include "util/stringbuilder.h"
#include <crypto/secp256k1.h>
#include <crypto/ecdsa.h>


msg_type_t msg_get_type(struct in3_client *c)
{
	int tokc, ret;
	jsmntok_t *tokv = 0;
	char *val = 0;
	msg_type_t type = T_ERROR;

	if (!c->msg->ready)
		return T_ERROR;

	ret = jsmnutil_parse_json(c->msg->data, &tokv, &tokc);
	val = get_json_key_value(c->msg->data, "msgType", tokv, tokc);

	if (tokv)
		k_free(tokv);

	if (val && !strcmp(val, "action"))
		type = T_ACTION;
	else if (val && !strcmp(val, "in3Response"))
		type = T_RESPONSE;

	if (val)
		k_free(val);

	c->msg->type = type;

	return type;
}

action_type_t msg_get_action(struct in3_client *c)
{
	int tokc, ret;
	jsmntok_t *tokv = 0;
	char *val = 0;
	action_type_t type = NONE;

	if (!c->msg->ready)
		return NONE ;

	if (c->msg->type != T_ACTION)
		return NONE;

	ret = jsmnutil_parse_json(c->msg->data, &tokv, &tokc);
	val = get_json_key_value(c->msg->data, "action", tokv, tokc);

	if (tokv)
		k_free(tokv);

	if (val && !strcmp(val, "unlock"))
		type = UNLOCK;
	else if (val && !strcmp(val, "lock"))
		type = LOCK;

	if (val)
		k_free(val);

	return type;
}

char *msg_get_response(struct in3_client *c)
{
	int tokc, ret;
	jsmntok_t *tokv = 0;
	char *val = 0;

	if (!c->msg->ready)
		return T_ERROR;

	ret = jsmnutil_parse_json(c->msg->data, &tokv, &tokc);
	val = get_json_key_value(c->msg->data, "responses", tokv, tokc);

	dbg_log("msg: '%s'\n", c->msg->data);
	if (tokv)
		k_free(tokv);

	return val;
}
static char *get_tx_hash(char *msg)
{
	int tokc, ret;
	jsmntok_t *tokv = 0;
	char *val = 0;

	ret = jsmnutil_parse_json(msg, &tokv, &tokc);

	val = get_json_key_value(msg, "transactionHash", tokv, tokc);
	if (tokv)
		k_free(tokv);

	if (!val)
		return NULL;

	if (strncmp(val, "0x", 2))
		goto err;

	return val;

err:
	if (val)
		k_free(val);

	return NULL;
}

int wait_for_message(struct in3_client *c)
{
	int i = 100;

	while (c->msg->ready == 0 && i--)
		k_sleep(100);

	k_mutex_lock(&c->mutex, 30000);
	k_mutex_unlock(&c->mutex);

	if (c->msg->ready)
		return 0;

	return i;
}

static struct in3_client *_client=NULL;

int send_ble(char** urls,int urls_len, char* pl, in3_response_t* result)  {


    char payload[5120];

	sprintf(payload,"{\"msgType\":\"in3Request\",\"msgId\":1,\"url\":\"%s\",\"method\":\"POST\",\"data\":%s}",*urls,pl);
	dbg_log("payload (len=%d): '%s'\n", strlen(payload), payload);

	clear_message(_client);
	bluetooth_write_req(payload);
	int err = wait_for_message(_client);
	bluetooth_clear_req();

	_client->txr->data = NULL;

	dbg_log("");
	if (err < 0) {
        sb_add_chars(&result->error, "Error receiving this response");
		return err;
	}
	dbg_log("");

	_client->txr->data = msg_get_response(_client);
    sb_add_chars(&result->result, _client->txr->data);
	return 0;
}


int in3_get_tx_receipt(struct in3_client *c, char *tx_hash, char **response)
{
	char params[100];
	char* result;
	char* error;

	if (!c || !tx_hash)
		return -1;

    _client=c;
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

	// random node for this op
//#define IN3_REQ_FMT "{\"msgType\":\"in3Request\",\"url\":\"%s\",\"method\":\"POST\",\"data\":{\"id\":100,\"jsonrpc\":\"2.0\",\"method\":\"eth_getTransactionReceipt\",\"params\":[\"%s\"],\"in3\":{\"verification\":\"proofWithSignature\",\"signatures\":[\"%s\"]}}}"


	sprintf(params, "[\"%s\"]", tx_hash);
    in3_client_rpc(c->in3,"eth_getTransactionReceipt",params,&result,&error);

    if (error) {
     	dbg_log("error: '%s'\n", error);
		*response =NULL;
		k_free(error);
	}
	else {
		*response = result;
		//TODO don't need it!
		c->txr->hash = k_calloc(1, strlen(tx_hash)+1);
		c->txr->hash = memcpy(c->txr->hash, tx_hash, strlen(tx_hash));
	}

	return 0;
}

int in3_can_rent(struct in3_client *c, char *resp, char *amsg)
{
	int ret = -1;
	int size, tokc, v;
	uint8_t *pubkey = 0, *sdata = 0;
	char tmp[256], mhash[256];
	char *logs = 0, *data = 0;
	char *topics = 0, *topic = 0, *log = 0;
	char *url = 0, *action = 0, *msg_hash = 0;
	char *r = 0, *s = 0, *vs = 0, *str = 0, *sig = 0;
	bytes_t *addr = 0, *signer = 0, *t = 0;
	jsmntok_t *tokv_l = 0, *tokv_t = 0;

	logs = json_get_value(resp, "logs");

	jsmnutil_parse_json(logs, &tokv_l, &tokc);

	int n = (tokv_l+1)->end - (tokv_l+1)->start;
	log = k_calloc(1, n * sizeof(char));
	memcpy(log, logs+(tokv_l+1)->start, n);

	topics = json_get_value(log, "topics");
	jsmnutil_parse_json(topics, &tokv_t, &tokc);

	size = jsmnutil_array_count(topics, tokv_t);
	jsmntok_t *k = tokv_t+1;

	topic = json_array_get_one_str(topics, &size, &k);
	if (strcmp(topic, "0x9123e6a7c5d144bd06140643c88de8e01adcbb24350190c02218a4435c7041f8"))
		goto out;

	data = json_get_value(log, "data");

	snprintf(tmp, 65,"%s", (data+2+64));
	c->rent->from = strtoul(tmp, NULL, 16);
	snprintf(tmp, 65, "%s", (data+2+(2*64)));
	c->rent->until = strtoul(tmp, NULL, 16);

	c->rent->controller = k_calloc(1, 43 * sizeof(char));
	snprintf(c->rent->controller, 43, "0x%s", (data+26));

	k_free(data);

	dbg_log("controller: '%s'\n", c->rent->controller);
	dbg_log("from: %d, until: %d, when: %d\n", c->rent->from, c->rent->until, c->rent->when);

	if ((c->rent->from >= c->rent->when) ||
		(c->rent->when >= c->rent->until)) {
		goto out;
	}

	dbg_log("amsg: '%s'\n", amsg);
	url = json_get_value(amsg, "url");
	action = json_get_value(amsg, "action");
	msg_hash = json_get_value(amsg, "messageHash");

	sig = json_get_value(amsg, "signature");
	r = json_get_value(sig, "r");
	s = json_get_value(sig, "s");
	vs = json_get_value(sig, "v");
	v = strtoul(vs, NULL, 16);
	k_free(vs);
	k_free(sig);

	sdata = k_calloc(1, 64 * sizeof(uint8_t));

	sprintf(tmp, "%s%s", r+2, s+2);
	str2byte_a(tmp, &sdata);
	k_free(r);
	k_free(s);

	sprintf(tmp, "%s%d%s{}", url, c->rent->when, action);
	sprintf(mhash, "\031Ethereum Signed Message:\n%d%s", strlen(tmp), tmp);
	dbg_log("mhash: '%s'\n", mhash);

	k_free(url);
	k_free(action);

	str2hex_str(mhash, &str);
	bytes_t *m = k_calloc(1, sizeof(bytes_t));
	m->len = str2byte_a(str, &m->data);
	k_free(str);

	bytes_t *hash = sha3(m);
	b_free(m);

	// compare msg hash
	t = k_calloc(1, sizeof(bytes_t));
	t->len = str2byte_a(msg_hash, &t->data);
	k_free(msg_hash);

	if (b_cmp(hash, t)) {
		b_free(hash);
		printk("Invalid message hash\n");
		goto out;
	}

	b_free(hash);
	hash = 0;

	pubkey = k_calloc(1, 65 * sizeof(uint8_t));
	if (v >= 27) {
		v -= 27;
	}

	// verify signature
	ecdsa_recover_pub_from_sig(&secp256k1, pubkey, sdata, t->data, v);
	b_free(t);
	t = 0;

	k_free(sdata);

	bytes_t *key = b_new(pubkey+1, 64);
	hash = sha3(key);
	b_free(key);

	// verify address here
	addr = b_new(hash->data+12, 20);
	signer = k_calloc(1, sizeof(bytes_t));
	signer->len = str2byte_a(c->rent->controller, &signer->data);
	b_free(hash);

	if (b_cmp(addr, signer)) {
		printk("Invalid signatory\n");
		goto out;
	}

	ret = 0;

out:
	if (topics)
		k_free(topics);
	if (topic)
		k_free(topic);
	if (tokv_t)
		k_free(tokv_t);
	if (tokv_l)
		k_free(tokv_l);
	if (log)
		k_free(log);
	if (logs)
		k_free(logs);
	if (t)
		b_free(t);
	if (addr)
		b_free(addr);
	if (signer)
		b_free(signer);
	if (pubkey)
		k_free(pubkey);

	return ret;
}

int verify_rent(struct in3_client *c)
{
	int ret = -1, id = 0;
	char *tx_hash = 0, *r = 0;
	char *amsg = 0;
	char payload[512];

	tx_hash = get_tx_hash(c->msg->data);
	if (!tx_hash)
		goto out;

	// keep copy of action message
	amsg = k_calloc(1, sizeof(char) * (c->msg->size + 1));
	amsg = memcpy(amsg, c->msg->data, c->msg->size + 1);
	id = json_get_int_value(amsg, "msgId");

	dbg_log("tx_hash: '%s'\n", tx_hash);

	in3_get_tx_receipt(c, tx_hash, &r);
	if (r)
		dbg_log("response: '%s'\n", r);

	if (in3_can_rent(c, r, amsg) < 0)
		goto out;

	ret = 0;

out:
	if (ret)
		sprintf(payload, "{\"msgId\":%d,\"msgType\":\"error\",\"error\":\"Invalid rental\"}", id);
	else
		sprintf(payload, "{\"msgId\":%d,\"msgType\":\"action\",\"result\":\"success\"}", id);

	dbg_log("payload (len=%d): '%s'\n", strlen(payload), payload);

	bluetooth_write_req(payload);

	dbg_log("c->txr: %p\n", c->txr);
	dbg_log("c->txr->hash: %p\n", c->txr->hash);
	dbg_log("c->txr->data: %p\n", c->txr->data);

	if (amsg)
		k_free(amsg);
	if (tx_hash)
		k_free(tx_hash);

	return ret;
}

void do_action(action_type_t action)
{
	if (action == LOCK) {
		printk("action: LOCK\n");
		for (int i = 4; i > 0; i--) {
			led_set(1);
			k_sleep(125);
			led_set(0);
			k_sleep(125);
		}
	} else {
		printk("action: UNLOCK\n");
		for (int i = 4; i > 0; i--) {
			led_set(0);
			k_sleep(125);
			led_set(1);
			k_sleep(125);
		}
	}
	gpio_set(action);
}
