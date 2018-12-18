#include <zephyr.h>
#include "client/client.h"

#include "util/bytes.h"

#ifndef LED0_GPIO_CONTROLLER
#define LED0_GPIO_CONTROLLER 	NORDIC_NRF_GPIO_50000000_LABEL
#define LED1_GPIO_CONTROLLER 	NORDIC_NRF_GPIO_50000300_LABEL
#endif
#define LED_PORT LED0_GPIO_CONTROLLER
#define LED		LED0_GPIO_PIN
#define LED_STRIP	15
#define ACTION_PIN	31

// Messaging


typedef enum {
	T_ERROR = 0,
	T_ACTION,
	T_REQUEST,
	T_RESPONSE
} msg_type_t;

typedef enum {
	NONE,
	UNLOCK,
	LOCK
} action_type_t;

typedef struct {
	int from;
	int until;
	int when;
	char *devid;
	char *controller;
} in3_rental_t;

typedef struct {
	char *hash;
	char *data;
} in3_tx_receipt_t;

// Client State Machine

typedef struct {
	int ready;
	int type;
	int size;
	char *data;
	u32_t start;
	u32_t end;
} in3_msg_t;

struct in3_client {
	in3_t* in3;
	struct k_sem sem;
	struct k_mutex mutex;
	in3_msg_t *msg;
	in3_rental_t *rent;
	in3_tx_receipt_t *txr;
};

void in3_signal_event(void);
int in3_client_start(void);

// Bluetooth and Board functions

int bluetooth_setup(struct in3_client *c);
void bluetooth_write_req(char *msg);
void bluetooth_clear_req(void);
int led_setup(void);
void led_set(int);
void gpio_set(action_type_t);
void clear_message(struct in3_client *c);
int in3_can_rent(struct in3_client *c, char *r, char *amsg);
