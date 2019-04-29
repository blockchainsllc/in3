//#include <zephyr.h>
//#include <kernel.h>
#include <stdio.h>
#include <string.h>
#include <misc/printk.h>
#include <misc/util.h>
//#include <util/data.h>
//#include <rtc.h>
//#include <client/client.h>
//#include <crypto/secp256k1.h>
//#include <crypto/ecdsa.h>
#include <settings/settings.h>
//#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>
//#include <bluetooth/hci.h>
//#include <bluetooth/conn.h>
//#include <bluetooth/l2cap.h>
//#include <bluetooth/rfcomm.h>
//#include <bluetooth/sdp.h>

#include <gpio.h>
#include <device.h>
#include <pwm.h>

#include "util/utils.h"
#include "util/bytes.h"
#include "util/debug.h"
#include "util/stringbuilder.h"
#include "client/client.h"
#include "eth_nano.h"

// GPIO definitions (USB & SDK)
#define CONFIG_GPIO_P0_DEV_NAME	"GPIO_0"
#define CONFIG_GPIO_P1_DEV_NAME	"GPIO_1"

#define IO_ON	1	// IO line ON (high level)
#define IO_OFF	0	// IO line OFF (low level)

#ifndef USB_DONGLE // if USB dongle is NOT enabled

 // SDK board I/O defines
 #define LEDPOWER_PORT	CONFIG_GPIO_P0_DEV_NAME
 #define LEDPOWER		13 // LED1 on PCA10056 (SDK)

 #define LEDSTRIP_PORT	CONFIG_GPIO_P0_DEV_NAME
 #define LEDSTRIP		14 // LED2 on PCA10056 (SDK)

 #define LOCKPIN_PORT	CONFIG_GPIO_P0_DEV_NAME
 #define LOCKPIN			15 // LED3 on PCA10056 (SDK)

#else // if USB dongle is used

 // USB board I/O defines
 #define LEDPOWER_PORT	CONFIG_GPIO_P0_DEV_NAME
 #define LEDPOWER		6 // LED1 on PCA10059 (USB)

 #define LEDSTRIP_PORT	CONFIG_GPIO_P1_DEV_NAME
 #define LEDSTRIP		15 // P1.15 on PCA10059 (USB)

 #define LOCKPIN_PORT	CONFIG_GPIO_P0_DEV_NAME
 #define LOCKPIN			31 // P0.31 on PCA10059 (USB)

 #define DOORPIN			2 // P0.02 PWM motor (door)

 // -> some others USB board useful I/Os
 #define LEDR_PORT	CONFIG_GPIO_P0_DEV_NAME
 #define LEDR		8 // LED2 Red on PCA10059 (USB)
 #define LEDG_PORT	CONFIG_GPIO_P1_DEV_NAME
 #define LEDG		9 // LED2 Green on PCA10059 (USB)
 #define LEDB_PORT	CONFIG_GPIO_P0_DEV_NAME
 #define LEDB		12 // LED2 Blue on PCA10059 (USB)

#endif

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

// prototypes of functions in main.c 
int bluetooth_setup(struct in3_client *c);
void bluetooth_write_req(char *msg);
void bluetooth_clear_req(void);
int start_message(int size);
void process_msg(void);
//int led_setup(void);
//void led_set(int);
//void gpio_set(action_type_t);
void clear_message(struct in3_client *c);
void door_control(int);
void ledpower_set(int);
void ledstrip_set(int);
void lock_set(int);
//void bt_dev_show_info(void);

// prototypes of functions in rent.c
int in3_can_rent(struct in3_client *c, char *r, char *amsg);
char *msg_get_payload(struct in3_client *c);
int verify_rent(struct in3_client *c);
// variables defined in rent.c
msg_type_t msg_get_type(char *c);
action_type_t msg_get_action(char *c);

// prototypes of functions in fsm.c
void in3_signal_event(void);
int in3_client_start(void);
void do_action(action_type_t action);
