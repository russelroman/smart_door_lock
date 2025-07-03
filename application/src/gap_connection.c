#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

#include "gap_advertising.h"
#include "gap_connection.h"


LOG_MODULE_REGISTER(gap_connection);


static void on_connected(struct bt_conn *conn, uint8_t err);
static void on_disconnected(struct bt_conn *conn, uint8_t reason);
static void on_security_changed(struct bt_conn *conn, bt_security_t level,
                                enum bt_security_err err);
static void on_le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency, 
							uint16_t timeout);
static void on_le_phy_updated(struct bt_conn *conn, struct bt_conn_le_phy_info *param);
static void on_le_data_len_updated(struct bt_conn *conn, struct bt_conn_le_data_len_info *info);


static struct bt_gatt_exchange_params exchange_params;
                                
struct bt_conn_cb connection_callbacks = 
{
	.connected = on_connected,
	.disconnected = on_disconnected,
	.security_changed = on_security_changed,
	.le_param_updated = on_le_param_updated,
	.le_phy_updated = on_le_phy_updated,
	.le_data_len_updated = on_le_data_len_updated,
};


void on_le_data_len_updated(struct bt_conn *conn, struct bt_conn_le_data_len_info *info)
{
    uint16_t tx_len     = info->tx_max_len; 
    uint16_t tx_time    = info->tx_max_time;
    uint16_t rx_len     = info->rx_max_len;
    uint16_t rx_time    = info->rx_max_time;

    LOG_INF("Data length updated. Length %d/%d bytes, time %d/%d us", tx_len, rx_len, 
				tx_time, rx_time);
}

static void exchange_func(struct bt_conn *conn, uint8_t att_err,
	struct bt_gatt_exchange_params *params)
{
	LOG_INF("MTU exchange %s", att_err == 0 ? "successful" : "failed");

	if (!att_err) 
	{
		uint16_t payload_mtu = bt_gatt_get_mtu(conn) - 3;   // 3 bytes used for Attribute headers.
		LOG_INF("New MTU: %d bytes", payload_mtu);
	}
}

static void update_mtu(struct bt_conn *conn)
{
    int err;
    exchange_params.func = exchange_func;

    err = bt_gatt_exchange_mtu(conn, &exchange_params);
    if (err) 
	{
        LOG_ERR("bt_gatt_exchange_mtu failed (err %d)", err);
    }
}

static void update_data_length(struct bt_conn *conn)
{
    int err;

    struct bt_conn_le_data_len_param my_data_len = 
	{
        .tx_max_len = BT_GAP_DATA_LEN_MAX,
        .tx_max_time = BT_GAP_DATA_TIME_MAX,
    };

    err = bt_conn_le_data_len_update(conn, &my_data_len);
    if (err) 
	{
        LOG_ERR("data_len_update failed (err %d)", err);
    }
}

static void on_le_phy_updated(struct bt_conn *conn, struct bt_conn_le_phy_info *param)
{
    if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_1M)
	{
        LOG_INF("PHY updated. New PHY: 1M");
    }
    else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_2M) 
	{
        LOG_INF("PHY updated. New PHY: 2M");
    }
    else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_CODED_S8) 
	{
        LOG_INF("PHY updated. New PHY: Long Range");
    }
}

static void update_phy(struct bt_conn *conn)
{
    int err;

    const struct bt_conn_le_phy_param preferred_phy = 
	{
        .options = BT_CONN_LE_PHY_OPT_NONE,
        .pref_rx_phy = BT_GAP_LE_PHY_2M,
        .pref_tx_phy = BT_GAP_LE_PHY_2M,
    };

    err = bt_conn_le_phy_update(conn, &preferred_phy);
    if (err) 
	{
        LOG_ERR("bt_conn_le_phy_update() returned %d", err);
    }
}

static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) 
	{
		LOG_INF("Connection failed (err %u)\n", err);
		return;
	}

	struct bt_conn_info info;
	err = bt_conn_get_info(conn, &info);

	uint32_t connection_interval_ms = info.le.interval * 1.25;
	uint16_t supervision_timeout_ms = info.le.timeout * 10;
	LOG_INF("Connection parameters: interval %d ms, latency %d intervals, timeout %d ms", 	\
						connection_interval_ms, info.le.latency, supervision_timeout_ms);

	LOG_INF("Connected");
	k_sleep(K_MSEC(1000));		/* Wait for connection process to fully complete */
	update_phy(conn);
	update_data_length(conn);
	update_mtu(conn);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason %u)\n", reason);
	advetising_start();
}

static void on_security_changed(struct bt_conn *conn, bt_security_t level,
                                enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err) 
    {
        LOG_INF("Security changed: %s level %u\n", addr, level);
    } 
    else 
    {
        LOG_INF("Security failed: %s level %u err %d\n", addr, level, err);
    }
}

static void on_le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
    uint32_t connection_interval_ms = interval * 1.25;         
    uint16_t supervision_timeout_ms = timeout * 10;          
    LOG_INF("Connection parameters updated: interval %d ms, latency %d intervals, timeout %d ms", 
				connection_interval_ms, latency, supervision_timeout_ms);
}