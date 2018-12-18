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

#include "jsmn/jsmnutil.h"
#include "fsm.h"
#include "util/utils.h"
#include "util/bytes.h"

#include "util/debug.h"


#define CONFIG_GPIO_P0_DEV_NAME              "GPIO_0"
#define CONFIG_GPIO_P1_DEV_NAME              "GPIO_1"

static struct bt_conn *default_conn = NULL;

void bt_dev_show_info(void);

static struct bt_uuid_128 msg_svc_uuid = BT_UUID_INIT_128(
	0x01, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static const struct bt_uuid_128 msg_char_uuid = BT_UUID_INIT_128(
	0x02, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static ssize_t read_msg(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;
	u16_t value_len;

	value_len = min(strlen(value), 512);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 value_len);
}

static const struct bt_uuid_128 req_char_uuid = BT_UUID_INIT_128(
	0x02, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x13);

static const struct bt_uuid_128 notify_uuid = BT_UUID_INIT_128(
	0x02, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x14);

static ssize_t read_req(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;
	u16_t value_len;

	value_len = min(strlen(value), 512);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 value_len);
}

static u32_t cnt;
struct device *led;
struct device *led_strip;
struct device *gpio;

static u8_t recv_buf_static[512];
static u8_t req_buf[512];
static u8_t *recv_buf;
static int b_off;
static unsigned long total_len;

static u32_t start, end;
static struct in3_client *client;

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
		printk("Error allocating RX buf (size=%lu)\n", total_len);
		return -1;
	}

	start = k_uptime_get_32();

	k_mutex_lock(&client->mutex, 5000);

	return 0;
}

void process_msg(void)
{
	printk("Received %lu bytes in %lums, c->msg=%p\n", total_len, (unsigned long) end-start, client->msg);
	dbg_log("Data:\n'%s'\n", recv_buf);
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

static ssize_t write_msg(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, u16_t len, u16_t offset,
			 u8_t flags)
{
	// if processing a message dont take new ones
	if (client->msg->ready)
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);

	if (total_len == 0) {
		int mtu = bt_gatt_get_mtu(conn) - 3;
		printk("GATT MTU: %d, DATA_SIZE: %d len=%d\n", mtu+3, mtu, len);
	}

	if (len == 0x8) {
		char magic[17];
		char *header = (char *) buf;
		if (total_len > 0) {
			printk("Channel already open, dismissing new request\n");
			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}
		sprintf(magic, "%02x%02x%02x%02x%02x%02x%02x%02x", header[0], header[1],
			header[2], header[3], header[4], header[5], header[6], header[7]);
		if (header[0] == 0x69 &&
		    header[1] == 0x6e &&
		    header[2] == 0x33 &&
		    header[3] == 0x63) {
			printk("Magic: 0x%s\n", magic);
			if (start_message(*((int *) buf+1)))
				return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
			bluetooth_clear_req();
			return len;
		}
	}

	memcpy(recv_buf + b_off, buf, len);
	b_off += len;
	dbg_log("copied to %p at offset %d len %hu\n", recv_buf, b_off, len);

	if (b_off >= total_len) {
		end = k_uptime_get_32();
		process_msg();
	}

	return len;
}

static struct bt_gatt_ccc_cfg req_ccc_cfg[BT_GATT_CCC_MAX] = {};

static void req_ccc_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	dbg_log("registered for REQ notification\n");
}

static struct bt_gatt_attr msg_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(&msg_svc_uuid),

	BT_GATT_CHARACTERISTIC(&msg_char_uuid.uuid,
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE,
			       read_msg, write_msg, recv_buf_static),

	BT_GATT_CHARACTERISTIC(&req_char_uuid.uuid,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_req, NULL, req_buf),

	BT_GATT_CHARACTERISTIC(&notify_uuid.uuid, BT_GATT_CHRC_NOTIFY,
				BT_GATT_PERM_NONE, NULL, NULL, NULL),

	BT_GATT_CCC(req_ccc_cfg, req_ccc_cfg_changed),
};

static struct bt_gatt_service msg_svc = BT_GATT_SERVICE(msg_attrs);

static const struct bt_data ad_discov[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static void connected(struct bt_conn *conn, u8_t err)
{
	default_conn = conn;
	printk("Connected\n");
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	printk("Disconnected\n");
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	int mtu = bt_gatt_get_mtu(conn);
	printk("Negotiating MTU to '%d'\n", mtu);

	return true;
}

static void le_param_updated(struct bt_conn *conn, u16_t interval,
			     u16_t latency, u16_t timeout)
{
	int mtu = bt_gatt_get_mtu(conn);
	printk("Negotiating MTU to '%d'\n", mtu);

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

	if (strlen(msg) > sizeof(recv_buf_static))
		return;

	strcpy(req_buf, msg);

	k_mutex_unlock(&client->mutex);

	dbg_log("send notification for REQ\n");

	char *send = "REQ_READY";
	bt_gatt_notify(NULL, &msg_attrs[5], send, strlen(send));
}

void bluetooth_clear_req(void)
{
	memset(req_buf, 0, sizeof(recv_buf_static));
}

int bluetooth_setup(struct in3_client *c)
{
	int err;
	size_t ad_len, scan_rsp_len = 0;
	struct bt_le_adv_param param;
	const struct bt_data *ad = 0, *scan_rsp = 0;

    // Enable to hardcode MAC address
	err = bt_enable(bt_ready);
	if (err) {
		printk("Unable to setup bluetooth");
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

	bt_set_name("in3-peripheral");

	err = bt_le_adv_start(&param, ad, ad_len, scan_rsp, scan_rsp_len);
	if (err < 0) {
		printk("Failed to start advertising (err %d)\n", err);
		return -1;
	} else {
		printk("Advertising started\n");
	}

	// set client data
	client = c;
	client->msg->data = recv_buf;

	recv_buf = 0;
	b_off = 0;
	total_len = 0;

	printk("Registering GATT service\n");
	err = bt_gatt_service_register(&msg_svc);

	if (err < 0) {
		printk("Failed to register servic\n");
		return -1;
	} else {
		printk("Service registered\n");
	}

	return 0;
}

void gpio_set(action_type_t state)
{
	int value = (state == UNLOCK) ? 1 : 0;

	gpio_pin_write(led, ACTION_PIN, value);
}

void led_set(int state)
{
	int value = state ? 0 : 1;
	gpio_pin_write(led, LED, value);
	gpio_pin_write(led_strip, LED_STRIP, state);
}

int led_setup(void)
{
	led = device_get_binding(CONFIG_GPIO_P0_DEV_NAME);
	led_strip = device_get_binding(CONFIG_GPIO_P1_DEV_NAME);

	gpio_pin_configure(led, LED, GPIO_DIR_OUT);
	gpio_pin_configure(led_strip, LED_STRIP, GPIO_DIR_OUT);

	gpio = device_get_binding(CONFIG_GPIO_P0_DEV_NAME);
	gpio_pin_configure(gpio, ACTION_PIN, GPIO_DIR_OUT);

	cnt = 0;

	for (int i = 0; i < 5; i++, cnt++) {
		led_set(cnt % 2);
		k_sleep(200);
	}

	led_set(0);
	gpio_set(LOCK);

	return 0;
}

void main(void)
{
	printk("Starting in3_client...\n");
	in3_client_start();

	return;
}
