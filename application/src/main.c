/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/settings/settings.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <math.h>

#include <app_event_manager.h>

#define MODULE main
LOG_MODULE_REGISTER(MODULE);
#include <caf/events/module_state_event.h>

#include "lock_svc.h"


#define BT_LE_ADV_CONN_NO_ACCEPT_LIST  BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE|BT_LE_ADV_OPT_ONE_TIME | BT_LE_ADV_OPT_USE_IDENTITY , \
				       BT_GAP_ADV_FAST_INT_MIN_2, \
				       BT_GAP_ADV_FAST_INT_MAX_2, NULL)

#define BT_LE_ADV_CONN_ACCEPT_LIST BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE|BT_LE_ADV_OPT_FILTER_CONN|BT_LE_ADV_OPT_ONE_TIME | BT_LE_ADV_OPT_USE_IDENTITY , \
				       BT_GAP_ADV_FAST_INT_MIN_2, \
				       BT_GAP_ADV_FAST_INT_MAX_2, NULL)

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_LED_BLINK_INTERVAL 1000
#define BUTTON_NODE DT_ALIAS(sw4)



static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),

};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LOCK_VAL),
};

K_MSGQ_DEFINE(device_message_queue, sizeof(uint16_t), 10, 2);

static const struct gpio_dt_spec button_temp = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb_data;
static bool is_bond_delete = 0;


static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	(BT_LE_ADV_OPT_CONNECTABLE |
	 BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
	800, /* Min Advertising Interval 500ms (800*0.625ms) */
	801, /* Max Advertising Interval 500.625ms (801*0.625ms) */
	NULL); /* Set to NULL for undirected advertising */


static void setup_accept_list_cb(const struct bt_bond_info *info, void *user_data)
{
	int *bond_cnt = user_data;
	if ((*bond_cnt) < 0) {
		return;
	}
	int err = bt_le_filter_accept_list_add(&info->addr);
	LOG_INF("Added following peer to whitelist: %x %x \n",info->addr.a.val[0],info->addr.a.val[1]);
	if (err) {
		LOG_INF("Cannot add peer to Filter Accept List (err: %d)\n", err);
		(*bond_cnt) = -EIO;
	} else {
		(*bond_cnt)++;
	}
}


static int setup_accept_list(uint8_t local_id)
{
	int err = bt_le_filter_accept_list_clear();
	if (err) {
		LOG_INF("Cannot clear Filter Accept List (err: %d)\n", err);
		return err;
	}
	int bond_cnt = 0;
	bt_foreach_bond(local_id, setup_accept_list_cb, &bond_cnt);
	return bond_cnt;
}


void advertise_with_acceptlist(struct k_work *work)
{
	int err=0;
	int allowed_cnt= setup_accept_list(BT_ID_DEFAULT);
	if (allowed_cnt<0){
		LOG_INF("Acceptlist setup failed (err:%d)\n", allowed_cnt);
	} else {
		if (allowed_cnt==0){
			LOG_INF("Advertising with no Filter Accept list\n"); 
			err = bt_le_adv_start(BT_LE_ADV_CONN_NO_ACCEPT_LIST, ad, ARRAY_SIZE(ad),
					sd, ARRAY_SIZE(sd));
		}
		else {
			LOG_INF("Acceptlist setup number  = %d \n",allowed_cnt);
			err = bt_le_adv_start(BT_LE_ADV_CONN_ACCEPT_LIST, ad, ARRAY_SIZE(ad),
				sd, ARRAY_SIZE(sd));	
		}
		if (err) {
		 	LOG_INF("Advertising failed to start (err %d)\n", err);
			return;
		}
		LOG_INF("Advertising successfully started\n");
	}
}
K_WORK_DEFINE(advertise_acceptlist_work, advertise_with_acceptlist);


static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	printk("Connected\n");
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);
	k_work_submit(&advertise_acceptlist_work);
}

static void on_security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u\n", addr, level);
	} else {
		LOG_INF("Security failed: %s level %u err %d\n", addr, level,
			err);
	}
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Pairing cancelled: %s\n", addr);
}

static void auth_passkey_entry(struct bt_conn *conn)
{
	LOG_INF("Passkey");

	int err;
	uint16_t temp;
	uint32_t passkey = 0;

	for(int i = 0; i < 6; ++i)
	{
		err = k_msgq_get(&device_message_queue, &temp, K_FOREVER);

		passkey = (temp - 48) * pow(10, 5 - i) + passkey;

		if(err == 0)
		{
			LOG_INF("Button Pressed: %c", (char)temp);
		}
	}

	LOG_INF("Passkey is: %d", passkey);

	err = bt_conn_auth_passkey_entry(conn, passkey);
	if(err != 0)
	{
		LOG_INF("Error on passkey entry: %d", err);
	}
		
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.passkey_entry = auth_passkey_entry,
	.cancel = auth_cancel,
};

struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
	.security_changed = on_security_changed,
};


void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    printk("Button pressed at pin %d\n", button_temp.pin);

	is_bond_delete = true;
}

int main(void)
{
	int err;
	
    if (!device_is_ready(button_temp.port)) {
        printk("Error: button device %s is not ready\n", button_temp.port->name);
        return -1;
    }

    err = gpio_pin_configure_dt(&button_temp, GPIO_INPUT);
    if (err != 0) {
        printk("Error %d: failed to configure %s pin %d\n", err, button_temp.port->name, button_temp.pin);
        return -1;
    }

    err = gpio_pin_interrupt_configure_dt(&button_temp, GPIO_INT_EDGE_TO_ACTIVE);
    if (err != 0) {
        printk("Error %d: failed to configure interrupt on %s pin %d\n", err, button_temp.port->name, button_temp.pin);
        return -1;
    }

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button_temp.pin));
    gpio_add_callback(button_temp.port, &button_cb_data);

    printk("Set up button at %s pin %d\n", button_temp.port->name, button_temp.pin);

	if (app_event_manager_init()) {
		LOG_ERR("Application Event Manager not initialized");
	} else {
		module_set_state(MODULE_STATE_READY);
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return -1;
	}
	bt_conn_cb_register(&connection_callbacks);

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		LOG_INF("Failed to register authorization callbacks.\n");
		return -1;
	}

	// Restore previous BLE bonds.
	settings_load();

	LOG_INF("Bluetooth initialized\n");

	k_work_submit(&advertise_acceptlist_work);

	LOG_INF("Advertising successfully started\n");

	uint16_t temp;

	for (;;) {

		if(is_bond_delete == 1)
		{
			is_bond_delete = 0;
			err = bt_unpair(BT_ID_DEFAULT,BT_ADDR_LE_ANY);

			if (err) {
				LOG_INF("Cannot delete bond (err: %d)\n", err);
			} else	{
				LOG_INF("Bond deleted succesfully \n");
			}	
		}	

		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
