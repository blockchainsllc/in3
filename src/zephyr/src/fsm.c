#include <kernel.h>
#include <misc/printk.h>
#include <stdio.h>
#include <string.h>

#include "fsm.h"
#include "util/utils.h"
#include <eth_full.h>
#include <usn_api.h>

#include "util/debug.h"

// Global Client
struct in3_client* client;
struct k_timer *   timer, *timer1, *timer2, *timer3; // timer structures
int                undo;

// private
static void timer_expired(struct k_timer* work) {
  undo = 1;
  dbg_log("<--- timer expired\n");
  k_sem_give(&client->sem);
}

static void timer1_expired(struct k_timer* work) // one shot timer (monitor led)
{
  ledpower_set(IO_ON); // power led on
}

static void timer2_expired(struct k_timer* work) // one shot timer (led stripe)
{
  ledstrip_set(IO_OFF); // ledstrip off
}

static void timer3_expired(struct k_timer* work) // one shot timer (lock-coil)
{
  lock_set(IO_OFF); // lock off
}

static void wait_for_event(void) {
  if (!client)
    return;

  //  k_sem_take(&client->sem, 600000); // EFnote: 600000 mS = 600 sec = 10 min
  k_sem_take(&client->sem, K_SECONDS(60)); // 60 sec = 1 min
}

void do_action(action_type_t action) {
  ledpower_set(IO_OFF);          // power led off
  k_timer_start(timer1, 250, 0); // start timer 1 initial duration 250mS, period = 0
  if (action == LOCK) {
    dbg_log("<--- action: LOCK\n");
  } else {
    dbg_log("<--- action: UNLOCK\n");
    ledstrip_set(IO_ON);             // led on
    k_timer_start(timer2, 10000, 0); // start timer 2 initial duration 10*1000mS, period = 0
    lock_set(IO_ON);                 // lock on
    k_timer_start(timer3, 1500, 0);  // start timer 3 initial duration 1500mS, period = 0
  }
}

// PUBLIC API
void in3_signal_event(void) {
  if (!client)
    return;

  dbg_log("<--- signalling event\n");
  k_sem_give(&client->sem);
}

/////////////////////////////////////

/*******************/
/*  state machine  */
/*******************/

typedef enum {
  STATE_INIT,
  STATE_WAITING,
  STATE_ACTION,
  STATE_RESET,
  STATE_MAX
} in3_state_t;

typedef in3_state_t in3_state_func_t(void);

// wait_for_message - called by send_ble
static int wait_for_message(struct in3_client* c) {
  int i = 100;

  dbg_log("<--- wait_for_message\n");
  while (c->msg->ready == 0 && i--)
    k_sleep(100);

  k_mutex_lock(&c->mutex, 30000);
  k_mutex_unlock(&c->mutex);

  if (c->msg->ready) {
    dbg_log("<--- msg received!\n");
    return 0;
  }
  dbg_log("<--- timeout error!\n");
  return i; // -1 if timeout elapsed
}

int send_ble(char** urls, int urls_len, char* pl, in3_response_t* result) {
  char payload[strlen(pl) + 100];

  sprintf(payload, "{\"msgType\":\"in3Request\",\"msgId\":1,\"url\":\"%s\",\"method\":\"POST\",\"data\":%s}", *urls, pl);

  clear_message(client);
  bluetooth_write_req(payload);
  int err = wait_for_message(client);
  bluetooth_clear_req();

  if (err < 0) {
    sb_add_chars(&result->error, "Error receiving this response");
    return err;
  }

  if (!client->msg->ready)
    return T_ERROR;

  // get the response-data and add it to the rersult
  char* val = json_get_json_value(client->msg->data, "responses");
  sb_add_chars(&result->result, val);
  _free(val);
  return 0;
}

static in3_state_t in3_init(void) {
  client = k_calloc(1, sizeof(struct in3_client));
  timer  = k_calloc(1, sizeof(struct k_timer));
  timer1 = k_calloc(1, sizeof(struct k_timer)); // allocate 1 array element of K_timer size
  timer2 = k_calloc(1, sizeof(struct k_timer)); // allocate 1 array element of K_timer size
  timer3 = k_calloc(1, sizeof(struct k_timer)); // allocate 1 array element of K_timer size

  // configure the device
  client->conf.chain_id = 0x044d;
  client->conf.now      = 0;
  hex2byte_arr("0x85Ec283a3Ed4b66dF4da23656d4BF8A507383bca", -1, client->conf.contract, 20);

  // register one device
  usn_register_device(&client->conf, "in3-1@tobalaba");

  // configure the incubed client
  in3_register_eth_full();
  client->conf.c               = in3_new();
  client->conf.c->chainId      = client->conf.chain_id;
  client->conf.c->requestCount = 1;
  client->conf.c->max_attempts = 1;
  client->conf.c->transport    = send_ble;

  // prepare the message-buffer
  client->msg = k_calloc(1, sizeof(in3_msg_t));

  bluetooth_setup(client);
  gpio_setup();

  k_sem_init(&client->sem, 0, 1);
  k_mutex_init(&client->mutex);
  k_timer_init(timer, timer_expired, NULL);
  k_timer_init(timer1, timer1_expired, NULL); // init timer, callback for expired, callback for stopped
  k_timer_init(timer2, timer2_expired, NULL); // init timer, callback for expired, callback for stopped
  k_timer_init(timer3, timer3_expired, NULL); // init timer, callback for expired, callback for stopped

  return STATE_WAITING;
}

static in3_state_t in3_waiting(void) {
  k_mutex_lock(&client->mutex, 10000);
  if (client->msg->ready) {
    dbg_log("<--- data received (len=%i):\n\n%s\n\n", strlen(client->msg->data), client->msg->data);
    client->msg->start = k_uptime_get_32();
    // only if it contains msgType it is worth verifying
    if (strstr(client->msg->data, "msgType")) {
      k_mutex_unlock(&client->mutex);
      return STATE_ACTION;
    }
    client->msg->end = k_uptime_get_32();
    dbg_log("<--- total time: %lums\n", (unsigned long) client->msg->end - client->msg->start);
    clear_message(client);
  }

  k_mutex_unlock(&client->mutex);
  wait_for_event();

  if (undo) {
    do_action(LOCK);
    undo = 0;
  }

  return STATE_WAITING;
}

static in3_state_t in3_action(void) {
  int  err;
  char payload[100];

  // TODO if we can, we should try to set the real timestamp in client->conf->now, if now=0, it will take it from the message, which is not relieable.
  usn_msg_result_t result = usn_verify_message(&client->conf, client->msg->data);

  // send response
  if (!result.accepted)
    sprintf(payload, "{\"msgId\":%d,\"msgType\":\"error\",\"error\":\"%s\"}", result.id, result.error_msg);
  else
    sprintf(payload, "{\"msgId\":%d,\"msgType\":\"action\",\"result\":\"success\"}", result.id);
  bluetooth_write_req(payload);

  // execute
  if (result.accepted && result.msg_type == USN_ACTION) {
    if (strcmp(result.action, "lock"))
      do_action(LOCK);
    else if (strcmp(result.action, "unlock")) {
      do_action(UNLOCK);
      k_timer_start(timer, K_SECONDS(5), 0);
    }
  }

  return STATE_RESET;
}

static in3_state_t in3_reset(void) {
  client->msg->end = k_uptime_get_32();

  dbg_log("<--- Total time: %lums\n", (unsigned long) client->msg->end - client->msg->start);
  clear_message(client);

  return STATE_WAITING;
}

in3_state_func_t* const state_table[STATE_MAX] = {
    in3_init,
    in3_waiting,
    in3_action,
    in3_reset,
    NULL};

static in3_state_t run_state(in3_state_t state) {
  return state_table[state]();
}

int in3_client_start(void) {
  in3_state_t state = STATE_INIT;

  while (1) {
    switch (state) {
      case STATE_INIT:
        dbg_log("<--- INIT\n");
        break;
      case STATE_WAITING:
        dbg_log("<--- WAITING\n");
        break;
      case STATE_ACTION:
        dbg_log("<--- ACTION\n");
        break;
      case STATE_RESET:
        dbg_log("<--- RESET\n");
        break;
      default:
        dbg_log("<--- STATE MACHINE ERROR!\n");
        state = STATE_RESET; // force state to reset
        break;
    }
    state = run_state(state);
  }
}
