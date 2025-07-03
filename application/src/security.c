#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include "security.h"


LOG_MODULE_REGISTER(security);


extern struct k_msgq device_message_queue;
extern struct k_sem my_sem; 


static void auth_cancel(struct bt_conn *conn);
static void auth_passkey_entry(struct bt_conn *conn);
static void auth_passkey_confirm(struct bt_conn *conn);
enum bt_security_err pairing_accept_cb(struct bt_conn *conn, const struct 
                                        bt_conn_pairing_feat *const feat);


struct bt_conn_auth_cb conn_auth_callbacks = 
{
	.pairing_accept = NULL,
	.passkey_display = NULL,
	.passkey_confirm = NULL,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
};


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

static void auth_passkey_confirm(struct bt_conn *conn)
{

}

enum bt_security_err pairing_accept_cb(struct bt_conn *conn, const struct bt_conn_pairing_feat *const feat)
{
	if(feat->io_capability == BT_IO_NO_INPUT_OUTPUT)
	{
		LOG_INF("JUST WORKS, DO NOT ALLOW");
		return BT_SECURITY_ERR_PAIR_NOT_ALLOWED;
	}

	return 0;
}

