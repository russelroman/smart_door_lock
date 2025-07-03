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
#include <zephyr/bluetooth/services/bas.h>
#include <math.h>

#include <app_event_manager.h>

#define MODULE main
LOG_MODULE_REGISTER(MODULE);
#include <caf/events/module_state_event.h>

#include "gatt_lock_svc.h"
#include "gap_advertising.h"
#include "gap_connection.h"
#include "security.h"


#define RUN_LED_BLINK_INTERVAL 1000
#define BUTTON_NODE DT_ALIAS(sw4)

K_MSGQ_DEFINE(device_message_queue, sizeof(uint16_t), 10, 2);

struct k_sem my_sem;

K_SEM_DEFINE(my_sem, 0, 1);

int pass_code = 123456;

static uint8_t battery_level = 100;


void keypad_thread(void *, void *, void *)
{
	uint16_t key = 0;

	//err = k_msgq_get(&device_message_queue, &temp, K_FOREVER);
	int err;
	int temp;
	int passkey = 0;

	while(1)
	{
		LOG_INF("Start of Thread");

		k_sem_take(&my_sem, K_FOREVER);

		for(int i = 0; i < 6; ++i)
		{
			err = k_msgq_get(&device_message_queue, &temp, K_FOREVER);

			passkey = (temp - 48) * pow(10, 5 - i) + passkey;

			if(err == 0)
			{
				LOG_INF("Button Pressed: %c", (char)temp);
			}
		}

		if(pass_code == passkey)
		{
			LOG_INF("Correct PIN");
		}
		else
		{
			LOG_INF("Incorrect PIN");
		}

		LOG_INF("Passkey: %d", passkey);
		LOG_INF("Body of Thread");
		//k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}


K_THREAD_DEFINE(keypad, 1024,
	keypad_thread, NULL, NULL, NULL,
	1, 0, 0);


static const struct gpio_dt_spec button_temp = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb_data;
static bool is_bond_delete = 0;


void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    printk("Button pressed at pin %d\n", button_temp.pin);

	is_bond_delete = true;
}

void simulate_battery_level(void)
{
	if (--battery_level == 0)
	{
		battery_level = 100;
	}

	bt_bas_set_battery_level(battery_level);
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

	simulate_battery_level();

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

	advetising_start();

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
