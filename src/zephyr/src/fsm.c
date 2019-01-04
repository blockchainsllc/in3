#include <kernel.h>
#include <misc/printk.h>
#include <stdio.h>
#include <string.h>

#include "fsm.h"
#include "rent.h"
#include "util/utils.h"
#include <eth_nano.h>

#include "util/debug.h"

// Global Client
struct in3_client* client;
struct k_timer*    timer;
int                undo;

// private
static void timer_expired(struct k_timer* work) {
  undo = 1;
  dbg_log("timer expired\n");
  k_sem_give(&client->sem);
}

static void wait_for_event(void) {
  if (!client)
    return;

  k_sem_take(&client->sem, 600000);
}

// PUBLIC API
void in3_signal_event(void) {
  if (!client)
    return;

  dbg_log("signalling event\n");
  k_sem_give(&client->sem);
}

/////////////////////////////////////

/*******************/
/*  state machine  */
/*******************/

typedef enum {
  STATE_INIT,
  STATE_WAITING,
  STATE_VERIFY,
  STATE_ACTION,
  STATE_RESET,
  STATE_MAX
} in3_state_t;

typedef in3_state_t in3_state_func_t(void);

static in3_state_t in3_init(void) {
  client = k_calloc(1, sizeof(struct in3_client));
  timer  = k_calloc(1, sizeof(struct k_timer));

  client->in3               = in3_new();
  client->in3->chainId      = 0x044d;
  client->in3->requestCount = 1;

  client->txr = k_calloc(1, sizeof(in3_tx_receipt_t));
  client->msg = k_calloc(1, sizeof(in3_msg_t));

  in3_register_eth_nano();
  bluetooth_setup(client);
  led_setup();
  k_sem_init(&client->sem, 0, 1);
  k_mutex_init(&client->mutex);
  k_timer_init(timer, timer_expired, NULL);

  return STATE_WAITING;
}

static in3_state_t in3_waiting(void) {
  k_mutex_lock(&client->mutex, 10000);
  if (client->msg->ready) {
    client->msg->start = k_uptime_get_32();
    if (msg_get_type(client) == T_ACTION) {
      k_mutex_unlock(&client->mutex);
      return STATE_ACTION;
    }
    client->msg->end = k_uptime_get_32();
    printk("Total time: %lums\n", (unsigned long) client->msg->end - client->msg->start);
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

static in3_state_t in3_verifying(void) {
  return STATE_RESET;
}

static in3_state_t in3_action(void) {
  int           err;
  action_type_t action = msg_get_action(client);

  printk("Action: 0x%02x\n", action);
  if (action != LOCK && action != UNLOCK)
    return STATE_RESET;

  // reset rental
  if (client->rent) {
    k_free(client->rent->controller);
    k_free(client->rent);
  }

  client->rent       = k_calloc(1, sizeof(in3_rental_t));
  client->rent->when = json_get_int_value(client->msg->data, "timestamp");

  err = verify_rent(client);
  if (err) {
    printk("Invalid rental\n");
    return STATE_RESET;
  }

  do_action(action);

  if (action == UNLOCK)
    k_timer_start(timer, K_SECONDS(5), 0);

  return STATE_RESET;
}

static in3_state_t in3_reset(void) {
  client->msg->end = k_uptime_get_32();

  printk("Total time: %lums\n", (unsigned long) client->msg->end - client->msg->start);
  clear_message(client);

  return STATE_WAITING;
}

in3_state_func_t* const state_table[STATE_MAX] = {
    in3_init,
    in3_waiting,
    in3_verifying,
    in3_action,
    in3_reset};

static in3_state_t run_state(in3_state_t state) {
  return state_table[state]();
}

int in3_client_start(void) {
  in3_state_t state = STATE_INIT;

  while (1) {
    dbg_log("Going to state 0x%02x\n", state);
    state = run_state(state);
  }
}
