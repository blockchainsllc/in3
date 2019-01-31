#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <misc/util.h>
#include <kernel.h>
#include <misc/printk.h>
#include <string.h>
#include <gpio.h>
#include <device.h>

#include <settings/settings.h>

#include <bluetooth/hci.h>
#include <bluetooth/gatt.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>

#include "fsm.h"
#include "util/utils.h"
#include "util/bytes.h"

#include "util/debug.h"

// prototypes:
void bt_dev_show_info(void);
int start_message(int size);
void process_msg(void);

// defines:
#define CONFIG_GPIO_P0_DEV_NAME              "GPIO_0"
#define CONFIG_GPIO_P1_DEV_NAME              "GPIO_1"

// variables:
static struct bt_conn *default_conn = NULL;
static struct bt_gatt_ccc_cfg req_ccc_cfg[BT_GATT_CCC_MAX] = {};

static u32_t cnt;
struct device *ledpower; // green led (power)
struct device *ledstrip; // led strip (used as lamp)
struct device *lockpin; // lock out
struct device *gpio; // using blue led on usb board for testing

static u8_t recv_buf_static[512];
static u8_t req_buf[512];

static u8_t *recv_buf;
static u32_t b_off;
static u32_t total_len;

static u32_t start, end;
static struct in3_client *client;

/////////////////////////////
// callbacks for BT services:
//
static ssize_t read_req(struct bt_conn *conn, const struct bt_gatt_attr *attr, // callback from: BT_GATT_CHARACTERISTIC (req_char_uuid)
			void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;
	u16_t value_len;

	dbg_log("<--- read req (len=%u ofs=%u)\n", len, offset);

	value_len = min(strlen(value), sizeof(req_buf)); // limit the length to req_buf size

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, value_len);
}

static ssize_t rx_data(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, u16_t len, u16_t offset,
			 u8_t flags)
{
	dbg_log("<--- len:%u\n", len);
	// if processing a message don't take new ones
	if (client->msg->ready)
		{
		dbg_log("<--- write req rejected (another message being processed)!!!\n");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}
/*
	if (total_len == 0) {
		int mtu = bt_gatt_get_mtu(conn) - 3;
		dbg_log(" GATT MTU: %d, DATA_SIZE: %d len=%d\n", mtu+3, mtu, len);
	}
*/
	if (len == 0x8) {
		char magic[17];
		char *header = (char *) buf;
		if (total_len > 0) {
			dbg_log("<--- Channel already open, dismissing new request\n");
			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}
		sprintf(magic, "%02x%02x%02x%02x%02x%02x%02x%02x", header[0], header[1],
			header[2], header[3], header[4], header[5], header[6], header[7]);
		if (header[0] == 0x69 &&
		    header[1] == 0x6e &&
		    header[2] == 0x33 &&
		    header[3] == 0x63) {
			dbg_log("<--- Magic: 0x%s\n", magic);
			if (start_message(*((int *) buf+1)))
				{
				dbg_log("<--- offset error\n");
				return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
				}
			bluetooth_clear_req();
			return len;
		}
	}

	if(recv_buf == 0) // if no space allocated
		recv_buf = k_calloc(1, total_len);
	////////////////////////////////////////////////////

	memcpy(recv_buf + b_off, buf, len);
	b_off += len;

	if (b_off >= total_len) {
		end = k_uptime_get_32();
		process_msg();
	}

	return len;
}

static void req_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value) // Callback from: BT_GATT_CCC (req_ccc_cfg, req_ccc_cfg_changed)
{
	dbg_log("<--- value = %u\n", value);
}

// define an UUID for msg_svc (BT_GATT_PRIMARY_SERVICE)
static struct bt_uuid_128 msg_svc_uuid = BT_UUID_INIT_128(
	0x01, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

// define an UUID for msg_char (BT_GATT_CHARACTERISTIC #1)
static const struct bt_uuid_128 msg_char_uuid = BT_UUID_INIT_128(
	0x02, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

// define an UUID for req_char (BT_GATT_CHARACTERISTIC #2)
static const struct bt_uuid_128 req_char_uuid = BT_UUID_INIT_128(
	0x02, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x13);

// define an UUID for notify (BT_GATT_CHARACTERISTIC #3)
static const struct bt_uuid_128 notify_uuid = BT_UUID_INIT_128(
	0x02, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x14);

static struct bt_gatt_attr msg_attrs[] = {
		BT_GATT_PRIMARY_SERVICE(&msg_svc_uuid), // [0] primary service

////////////////////////////////////////////////////////////////////////
		BT_GATT_CHARACTERISTIC(&msg_char_uuid.uuid, // [1] characteristic attribute uuid
			BT_GATT_CHRC_WRITE, // char att properties
			BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE, // char att permissions
			NULL, rx_data, recv_buf_static), // char att read callback, write callback, value

		BT_GATT_CHARACTERISTIC(&req_char_uuid.uuid, // [2] characteristic attribute uuid
			BT_GATT_CHRC_READ, // char att properties
			BT_GATT_PERM_READ, // char att permissions
			read_req, NULL, req_buf), // char att read callback, write callback, value

		BT_GATT_CHARACTERISTIC(&notify_uuid.uuid, // [3] characteristic attribute uuid
			BT_GATT_CHRC_NOTIFY, // char att properties
			BT_GATT_PERM_NONE, // char att permissions
			NULL, NULL, NULL), // char att read callback, write callback, value
////////////////////////////////////////////////////////////////////////

		BT_GATT_CCC(req_ccc_cfg, req_ccc_cfg_changed), // [4] initial config, callback to configuration change
};

////////
// Code:
//
int start_message(int size)
{
	int old_len = total_len;
	total_len = size;
	dbg_log("RX Message (length: %lu bytes)\n", total_len);

	if (recv_buf && (old_len > total_len)) {
		memset(recv_buf, 0, total_len);
		goto out;
	}

	if (recv_buf)
		k_free(recv_buf);

	recv_buf = k_calloc(1, total_len * sizeof(char));

out:
	if (!recv_buf) {
		dbg_log("Error allocating RX buf (size=%lu)\n", total_len);
		return -1;
	}

	start = k_uptime_get_32();

	k_mutex_lock(&client->mutex, 5000);

	return 0;
}

void process_msg(void)
{
	client->msg->data = recv_buf;
	client->msg->size = total_len;

	client->msg->ready = 1;

	b_off = 0;
	total_len = 0;

	k_mutex_unlock(&client->mutex);
	in3_signal_event();
}

void clear_message(struct in3_client *c)
{
	// EFmod: in case of raw message received, msg->ready flag will NOT be reset
	//  and this may hang the program; so, do this anyway:
	c->msg->ready = 0; // EFmod - may be also this is unassigned

	if (!recv_buf)
		return;

	k_free(recv_buf);

	c->msg->size = 0;
	c->msg->ready = 0;

	b_off = 0;
	total_len = 0;

	recv_buf = 0;
	c->msg->data = 0;
}

static struct bt_gatt_service msg_svc = BT_GATT_SERVICE(msg_attrs);

static const struct bt_data ad_discov[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static void connected(struct bt_conn *conn, u8_t err)
{
	dbg_log("<---\n"); // function is shown on the left
	default_conn = conn;
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	dbg_log("<---\n"); // function is shown on the left
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
int mtu = bt_gatt_get_mtu(conn);

	dbg_log("<--- req MTU=%d\n", mtu);
	return true;
}

static void le_param_updated(struct bt_conn *conn, u16_t interval, u16_t latency, u16_t timeout)
{
int mtu = bt_gatt_get_mtu(conn);

	dbg_log("<--- upd MTU=%d\n", mtu);
	return;
}

static struct bt_conn_cb conn_cbs = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
};

static void bt_ready(int err)
{
	default_conn = NULL;
	bt_conn_cb_register(&conn_cbs);
}

void bluetooth_write_req(char *msg)
{
	k_mutex_lock(&client->mutex, 5000);

	if (strlen(msg) > sizeof(recv_buf_static)) // EFmod: may be req_buf ?
		return;

	strcpy(req_buf, msg);
	dbg_log("<--- buffer prepared for req_char:\n%s\n", req_buf);

	k_mutex_unlock(&client->mutex);

	dbg_log("send notification for REQ\n");

	char *send = "REQ_READY";
//  EFmod: the "normal" [3] does not work. Index [5] is out of bounds, but it works...
	bt_gatt_notify(NULL, &msg_attrs[5], send, strlen(send));
}

void bluetooth_clear_req(void)
{
	memset(req_buf, 0, sizeof(recv_buf_static)); // EFmod: may be sizeof(req_buf) ?
}

// START block: next 3 functions needed only if hardcoded mac is requested
static int char2hex(const char *c, u8_t *x)
{
	if (*c >= '0' && *c <= '9') {
		*x = *c - '0';
	} else if (*c >= 'a' && *c <= 'f') {
		*x = *c - 'a' + 10;
	} else if (*c >= 'A' && *c <= 'F') {
		*x = *c - 'A' + 10;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int str2bt_addr(const char *str, bt_addr_t *addr)
{
	int i, j;
	u8_t tmp;

	if (strlen(str) != 17) {
		return -EINVAL;
	}

	for (i = 5, j = 1; *str != '\0'; str++, j++) {
		if (!(j % 3) && (*str != ':')) {
			return -EINVAL;
		} else if (*str == ':') {
			i--;
			continue;
		}

		addr->val[i] = addr->val[i] << 4;

		if (char2hex(str, &tmp) < 0) {
			return -EINVAL;
		}

		addr->val[i] |= tmp;
	}

	return 0;
}

static int str2bt_addr_le(const char *str, const char *type, bt_addr_le_t *addr)
{
	int err;

	err = str2bt_addr(str, &addr->a);
	if (err < 0) {
		return err;
	}

	if (!strcmp(type, "public") || !strcmp(type, "(public)")) {
		addr->type = BT_ADDR_LE_PUBLIC;
	} else if (!strcmp(type, "random") || !strcmp(type, "(random)")) {
		addr->type = BT_ADDR_LE_RANDOM;
	} else {
		return -EINVAL;
	}

	return 0;
}
// END block

int bluetooth_setup(struct in3_client *c)
{
	int err;
	size_t ad_len, scan_rsp_len = 0;
	struct bt_le_adv_param param;
	const struct bt_data *ad = 0, *scan_rsp = 0;

// Enable to hardcode MAC address
#if BTMAC
	// client (this)
	bt_addr_le_t addr;
//	char *c_addr = "d3:6c:29:4b:6b:45";
	char *c_addr = "c0:00:00:00:00:00"; // EFmod:
	char *c_addr_type = "(random)";
	err = str2bt_addr_le(c_addr, c_addr_type, &addr);
	bt_set_id_addr((const bt_addr_le_t *) &addr);
#endif

	err = bt_enable(bt_ready);
	if (err) {
		dbg_log("Unable to setup bluetooth");
		return -1;
	}

	k_sleep(1000);
	bt_dev_show_info();

	param.id = 0;
	param.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
	param.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;
	param.options = (BT_LE_ADV_OPT_CONNECTABLE |
				 BT_LE_ADV_OPT_USE_NAME);
	ad = ad_discov;
	ad_len = ARRAY_SIZE(ad_discov);

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

//	bt_set_name("in3-peripheral");
	bt_set_name("in3-emiliotest"); // EFmod: just for testing with Ardo's App

	err = bt_le_adv_start(&param, ad, ad_len, scan_rsp, scan_rsp_len);
	if (err < 0) {
		dbg_log("Failed to start advertising (err %d)\n", err);
		return -1;
	} else {
		dbg_log("Advertising started\n");
	}

	// set client data
	client = c;
	client->msg->data = recv_buf;

	recv_buf = 0;
	b_off = 0;
	total_len = 0;

	dbg_log("Registering GATT service\n");
	err = bt_gatt_service_register(&msg_svc);

	if (err < 0) {
		dbg_log("Failed to register servic\n");
		return -1;
	} else {
		dbg_log("Service registered\n");
	}

	return 0;
}

///////// new GPIO functions
void ledpower_set(int state)
{
	if(state) // if ON
		gpio_pin_write(ledpower, LEDPOWER, 0); // led with negative logic
	else // if OFF
		gpio_pin_write(ledpower, LEDPOWER, 1); // led with negative logic
}

void ledstrip_set(int state)
{
	if(state) // if ON
		gpio_pin_write(ledstrip, LEDSTRIP, 1); // led with positive logic
	else // if OFF
		gpio_pin_write(ledstrip, LEDSTRIP, 0); // led with positive logic
}

void lock_set(int state)
{
	if(state) // if ON
		gpio_pin_write(lockpin, LOCKPIN, 1); // coil with positive logic
	else // if OFF
		gpio_pin_write(lockpin, LOCKPIN, 0); // coil with positive logic
}

int gpio_setup(void)
{
	ledpower = device_get_binding(LEDPOWER_PORT); // CONFIG_GPIO_P0_DEV_NAME
	gpio_pin_configure(ledpower, LEDPOWER, GPIO_DIR_OUT); // set as output (see fsm.h)
	ledpower_set(IO_ON);

	ledstrip = device_get_binding(LEDSTRIP_PORT);
	gpio_pin_configure(ledstrip, LEDSTRIP, GPIO_DIR_OUT); // set as output (see fsm.h)
	ledstrip_set(IO_OFF);

	lockpin = device_get_binding(LOCKPIN_PORT);
	gpio_pin_configure(lockpin, LOCKPIN, GPIO_DIR_OUT); // set as output (see fsm.h)
	lock_set(IO_OFF);

	for(int i = 0, cnt = 0; i < 5; i++, cnt++) // blink led and terminate ON
		{
		ledpower_set(cnt % 2);
		k_sleep(200);
		}
	ledpower_set(IO_ON);
	return 0;
}
/////////////////////// end of new GPIO functions
void main(void)
{
	dbg_log("\n\n\n\n\n***\n*** Starting in3_client...\n");
	in3_client_start();

	return;
}
