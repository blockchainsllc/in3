#include <stdio.h>
#include <string.h>
#include <rtc.h>
#include <client/client.h>
#include "fsm.h"
#include "util/debug.h"
#include "util/stringbuilder.h"
#include <crypto/secp256k1.h>
#include <crypto/ecdsa.h>
#include <util/data.h>
#include <eth_nano.h>



msg_type_t msg_get_type(struct in3_client *c)
{
	char *val = 0;
	msg_type_t type = T_ERROR;

	if (!c->msg->ready)
		return T_ERROR;

    val = json_get_str_value(c->msg->data, "msgType");
	if (val && !strcmp(val, "action"))
		type = T_ACTION;
	else if (val && !strcmp(val, "in3Response"))
		type = T_RESPONSE;

	if (val)
		_free(val);

	c->msg->type = type;

	return type;
}

action_type_t msg_get_action(struct in3_client *c)
{
	char *val = 0;
	action_type_t type = NONE;

	if (!c->msg->ready)
		return NONE ;

	if (c->msg->type != T_ACTION)
		return NONE;

    val = json_get_str_value(c->msg->data, "action");


	if (val && !strcmp(val, "unlock"))
		type = UNLOCK;
	else if (val && !strcmp(val, "lock"))
		type = LOCK;

	if (val)
		_free(val);

	return type;
}

char *msg_get_response(struct in3_client *c)
{
	char *val = 0;

	if (!c->msg->ready)
		return T_ERROR;

	val = json_get_json_value(c->msg->data, "responses");
	dbg_log("msg: '%s'\n", c->msg->data);
	return val;
}
static char *get_tx_hash(char *msg)
{
	char *val = json_get_str_value(msg, "transactionHash");
	if (!val)
		return NULL;

	if (strncmp(val, "0x", 2))
		goto err;

	return val;

err:
	if (val)
		_free(val);

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





int in3_can_rent(struct in3_client *c, char *resp, char *amsg) {
	int res=-1,i;
    char tmp[256], mhash[256];
	json_parsed_t*  response = parse_json(resp);
	json_parsed_t*  message  = parse_json(amsg);
	d_token_t* l, *log=NULL;
	bytes_t* signer=NULL,  *hash=NULL;
	bytes_t* log_rented = hex2byte_new_bytes("9123e6a7c5d144bd06140643c88de8e01adcbb24350190c02218a4435c7041f8",64);


	if (!response || !message) goto out; 

	d_token_t* logs = d_get(response->items,key("logs"));
	if (!logs)  goto out;

	for (i=0,l=logs+1;i<d_len(logs);i++,l=d_next(l)) {
		if (b_cmp(d_get_bytes_at(d_get(l,key("topics")),0),log_rented)) {
			log=l;
			break;
		}
	}

    // no event found,
	if (!log) goto out; 

	char* url         = d_get_string(message->items,"url");
    //bytes_t* deviceId = d_get_bytes_at(d_get(log,key("topics")),1);  // we need to store and compare the deviceId
	bytes_t* data     = d_get_bytes(log,"data");
	
	c->rent->from  = bytes_to_long(data->data+32,32);
	c->rent->until = bytes_to_long(data->data+64,32);
    //TODO do we need to set it as string?
    c->rent->controller = _malloc(43);
    c->rent->controller[0]='0';
    c->rent->controller[1]='x';
	int8_to_char(data->data+12,20,c->rent->controller+2);


	dbg_log("controller: '%s'\n", c->rent->controller);
	dbg_log("from: %d, until: %d, when: %d\n", c->rent->from, c->rent->until, c->rent->when);

    // wrong time
	if (c->rent->from >= c->rent->when ||  c->rent->when >= c->rent->until)  goto out;

	// check signature


    // prepare message hash
	sprintf(tmp, "%s%d%s{}", url, c->rent->when, d_get_string(message->items,"action"));
	sprintf(mhash, "\031Ethereum Signed Message:\n%d%s", strlen(tmp), tmp);
	bytes_t msg= { .data = (uint8_t*) &mhash, .len = strlen(mhash) };
    hash = sha3(&msg);

    // get the signature
	signer = ecrecover_signature(hash,  d_get(message->items,key("signature")));
	if (signer==NULL) goto out;

    // check if the signer is the same as the controller in the event.
    // reuse msg to point to the address in the event.
	msg.data = data->data+12;
	msg.len  = 20;
	if (!b_cmp(signer, &msg))  goto out;

	//TODO check if the url and the deviceid is correct.
	// if (strcmp(url, saved_url)!=0)          goto out;
	// if (!b_comp(device_id, saved_device_id)) goto out;
	// if (!b_comp( d_get_bytes(response,"contractAddress") , saved_contract)) goto out;

	res = 0;
	
	out:
	if (response) _free(response);
	if (message)  _free(message);
	if (hash)     b_free(hash);
	if (signer)   b_free(signer);
	b_free(log_rented);

	return res;
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
