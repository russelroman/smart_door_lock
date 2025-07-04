/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_LBS_H_
#define BT_LBS_H_

/**@file
 * @defgroup bt_lock_svc Lock Service API
 * @{
 * @brief API for the Lock Service.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <zephyr/bluetooth/uuid.h>


#define BT_UUID_LOCK_VAL	\
	BT_UUID_128_ENCODE(0x1c376f00, 0x468f, 0xEFDE, 0x8652, 0x4183255a1de3)

#define BT_UUID_LOCK_PIN_VAL \
	BT_UUID_128_ENCODE(0x1c376f01, 0x468f, 0xEFDE, 0x8652, 0x4183255a1de3)

#define BT_UUID_LOCK_STATE_VAL \
	BT_UUID_128_ENCODE(0x1c376f02, 0x468f, 0xEFDE, 0x8652, 0x4183255a1de3)

#define BT_UUID_LOCK           BT_UUID_DECLARE_128(BT_UUID_LOCK_VAL)
#define BT_UUID_LOCK_PIN    BT_UUID_DECLARE_128(BT_UUID_LOCK_PIN_VAL)
#define BT_UUID_LOCK_STATE       BT_UUID_DECLARE_128(BT_UUID_LOCK_STATE_VAL)

/** @brief Callback type for when an LED state change is received. */
typedef void (*led_cb_t)(const bool led_state);

/** @brief Callback type for when the button state is pulled. */
typedef bool (*button_cb_t)(void);

/** @brief Callback struct used by the LBS Service. */
struct my_lbs_cb {
	/** LED state change callback. */
	led_cb_t led_cb;
	/** Button read callback. */
	button_cb_t button_cb;
};

/** @brief Initialize the LBS Service.
 *
 * This function registers application callback functions with the My LBS
 * Service
 *
 * @param[in] callbacks Struct containing pointers to callback functions
 *			used by the service. This pointer can be NULL
 *			if no callback functions are defined.
 *
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int my_lbs_init(struct my_lbs_cb *callbacks);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_LBS_H_ */
