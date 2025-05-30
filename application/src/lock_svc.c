/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief LED Button Service (LBS) sample
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "lock_svc.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(smart_door_lock);

static bool button_state;

static ssize_t write_led(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr,
			 const void *buf,
			 uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Attribute write, handle: %u, conn: %p", attr->handle,
		(void *)conn);

	if (len != 1U) {
		LOG_DBG("Write led: Incorrect data length");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (offset != 0) {
		LOG_DBG("Write led: Incorrect data offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	//Read the received value 
	uint8_t val = *((uint8_t *)buf);

	if (val == 0x00 || val == 0x01) {
		//Call the application callback function to update the LED state
		//lbs_cb.led_cb(val ? true : false);
	} else {
		LOG_DBG("Write led: Incorrect value");
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	return len;
}


static ssize_t read_button(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  void *buf,
			  uint16_t len,
			  uint16_t offset)
{
	//get a pointer to button_state which is passed in the BT_GATT_CHARACTERISTIC() and stored in attr->user_data
	const char *value = attr->user_data;

	LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle,
		(void *)conn);


	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
					sizeof(*value));
	
}

/* LED Button Service Declaration */
BT_GATT_SERVICE_DEFINE(
	lock_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_LOCK),
	BT_GATT_CHARACTERISTIC(BT_UUID_LOCK_PIN,
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE_AUTHEN,
			       NULL, write_led, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_LOCK_STATE,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ_AUTHEN, read_button, NULL,
			       &button_state),

);
