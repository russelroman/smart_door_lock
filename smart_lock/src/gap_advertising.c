#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>

#include "gatt_lock_svc.h"
#include "gap_advertising.h"


LOG_MODULE_REGISTER(gap_advertising);


#define BT_GAP_ADV_MIN_INTERVAL_STEP_MS		1.6000	
#define BT_GAP_ADV_MS_TO_INTERVAL(ADV_INTERVAL_MS)	((ADV_INTERVAL_MS) * (BT_GAP_ADV_MIN_INTERVAL_STEP_MS))

/* 
   Different minimum and maximum advertising interval
   will add randomness on the advertising interval. Thus,
   will decrease chance of packet collision.
*/
#define BT_GAP_ADV_INTERVAL_VARIATION	10
#define BT_GAP_ADV_INTERVAL_MIN_MS	100
#define BT_GAP_ADV_INTERVAL_MAX_MS	(BT_GAP_ADV_INTERVAL_MIN_MS + BT_GAP_ADV_INTERVAL_VARIATION)	

#define BT_GAP_ADV_INTERVAL_MIN		BT_GAP_ADV_MS_TO_INTERVAL(BT_GAP_ADV_INTERVAL_MIN_MS)
#define BT_GAP_ADV_INTERVAL_MAX		BT_GAP_ADV_MS_TO_INTERVAL(BT_GAP_ADV_INTERVAL_MAX_MS)

#define BT_LE_ADV_CONN_NO_ACCEPT_LIST_OPTIONS																\
			BT_LE_ADV_OPT_CONNECTABLE	|		   /* Connectable Advertising*/							    \
			BT_LE_ADV_OPT_ONE_TIME		|		   /* Advertise one time only*/								\
            BT_LE_ADV_OPT_USE_IDENTITY			   /* Use the identity address at advertister address*/		\

#define BT_LE_ADV_CONN_ACCEPT_LIST_OPTIONS																    \
			BT_LE_ADV_OPT_CONNECTABLE	|		   /* Connectable Advertising*/							    \
			BT_LE_ADV_OPT_ONE_TIME		|		   /* Advertise one time only*/								\
            BT_LE_ADV_OPT_USE_IDENTITY	|		   /* Use the identity address at advertister address*/		\
			BT_LE_ADV_OPT_FILTER_CONN			   /* Use filter accept list */								\

#define BT_LE_ADV_CONN_NO_ACCEPT_LIST  BT_LE_ADV_PARAM(BT_LE_ADV_CONN_NO_ACCEPT_LIST_OPTIONS, 	\
													   BT_GAP_ADV_INTERVAL_MIN , 				\
													   BT_GAP_ADV_INTERVAL_MIN , 				\
													   NULL)

#define BT_LE_ADV_CONN_ACCEPT_LIST BT_LE_ADV_PARAM(BT_LE_ADV_CONN_ACCEPT_LIST_OPTIONS,			\
												   BT_GAP_ADV_INTERVAL_MIN , 					\
	                                               BT_GAP_ADV_INTERVAL_MIN , 				    \
	                                               NULL)

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)


static const struct bt_data ad[] = 
{
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LOCK_VAL),
};


static void setup_accept_list_cb(const struct bt_bond_info *info, void *user_data)
{
	int *bond_cnt = user_data;

	if ((*bond_cnt) < 0) 
	{
		return;
	}

	int err = bt_le_filter_accept_list_add(&info->addr);
	LOG_INF("Added following peer to whitelist: %x %x \n",info->addr.a.val[0],info->addr.a.val[1]);

	if (err) 
	{
		LOG_INF("Cannot add peer to Filter Accept List (err: %d)\n", err);
		(*bond_cnt) = -EIO;
	} 
	else 
	{
		(*bond_cnt)++;
	}
}

static int setup_accept_list(uint8_t local_id)
{
	int err = bt_le_filter_accept_list_clear();

	if (err) 
	{
		LOG_INF("Cannot clear Filter Accept List (err: %d)\n", err);
		return err;
	}

	int bond_cnt = 0;
	bt_foreach_bond(local_id, setup_accept_list_cb, &bond_cnt);

	return bond_cnt;
}

/*
	The accept list is initialized and created
	before advertising.

	Advertising will be performed after disconnection.
	It is not advisable to call bt_le_adv_start() inside
	the disconnect callback. The disconnect callback runs
	on the Bluetooth RX Thread. Thus, the advertising
	must be done using work queues. The work on work queues
	will be performed on the thread context.
*/
void advertise_with_acceptlist(struct k_work *work)
{
	int err=0;
	int allowed_cnt= setup_accept_list(BT_ID_DEFAULT);

	if (allowed_cnt < 0)
	{
		LOG_INF("Acceptlist setup failed (err:%d)\n", allowed_cnt);
	} 
	else 
	{
		if (allowed_cnt == 0)
		{
			LOG_INF("Advertising with no Filter Accept list\n"); 
			err = bt_le_adv_start(BT_LE_ADV_CONN_NO_ACCEPT_LIST, ad, ARRAY_SIZE(ad),
					NULL, 0);
		}
		else 
		{
			LOG_INF("Acceptlist setup number  = %d \n",allowed_cnt);
			err = bt_le_adv_start(BT_LE_ADV_CONN_ACCEPT_LIST, ad, ARRAY_SIZE(ad),
				NULL, 0);	
		}

		if (err) 
		{
		 	LOG_INF("Advertising failed to start (err %d)\n", err);
			return;
		}
		
		LOG_INF("Advertising successfully started\n");
	}
}
K_WORK_DEFINE(advertise_acceptlist_work, advertise_with_acceptlist);

void advetising_start(void)
{
    k_work_submit(&advertise_acceptlist_work);
}