#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/printk.h>


#define BT_UUID_DIS_FW_REVISION      BT_UUID_DECLARE_16(0x2A26)
#define BT_UUID_DIS_SW_REVISION      BT_UUID_DECLARE_16(0x2A28)
#define BT_UUID_DIS_MANUFACTURER     BT_UUID_DECLARE_16(0x2A29)
#define BT_UUID_DIS_MODEL_NUM        BT_UUID_DECLARE_16(0x2A24)


static ssize_t read_extra_field(struct bt_conn *conn,
        const struct bt_gatt_attr *attr,
        void *buf, uint16_t len, uint16_t offset)
{
    const char *value = attr->user_data;
    return bt_gatt_attr_read(conn, attr, buf, len, offset, value, strlen(value));
}


BT_GATT_SERVICE_DEFINE(dis_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_16(0x180A)),

    BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER,
        BT_GATT_CHRC_READ,
        BT_GATT_PERM_READ,
        read_extra_field, NULL, "Diamond"),

    BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MODEL_NUM,
        BT_GATT_CHRC_READ,
        BT_GATT_PERM_READ,
        read_extra_field, NULL, "891242"),


    BT_GATT_CHARACTERISTIC(BT_UUID_DIS_FW_REVISION,
                           BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ,
                           read_extra_field, NULL, "1.0"),

    BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SW_REVISION,
                           BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ,
                           read_extra_field, NULL, "0.5")
);