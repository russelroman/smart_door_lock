/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

 #include <stdint.h>
 #include <zephyr/kernel.h>
 
#define MODULE keypad
#include <caf/events/module_state_event.h>
#include <caf/events/button_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, 3);

#define MAX_ROW	4
#define MAX_COLUMN	3

extern struct k_msgq device_message_queue; 

static char keypad_mapping[MAX_ROW][MAX_COLUMN] = 
{
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

enum button_id {
    BUTTON_ID_NEXT_EFFECT,
    BUTTON_ID_NEXT_LED,

    BUTTON_ID_COUNT
};

extern struct k_sem my_sem;

static bool handle_button_event(const struct button_event *evt)
{
    if (evt->pressed) {

        uint8_t row;
        uint8_t column;

        row = evt->key_id & (0x007FU);
        column = (evt->key_id & (0x3F80U)) >> 7;

        int ret = 0;

        uint16_t pressed;

        pressed = keypad_mapping[row][column];

        if(pressed == '#')
        {
            k_sem_give(&my_sem);
        }
        else
        {
            ret = k_msgq_put(&device_message_queue, &pressed , K_FOREVER);
        }

        if (ret){
            LOG_ERR("Return value from k_msgq_put = %d",ret);
        }

        LOG_INF("Row: %d, Column: %d", row, column);
        LOG_INF("Pressed: %c", keypad_mapping[row][column]);
    }

    return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
    if (is_button_event(aeh)) {
        return handle_button_event(cast_button_event(aeh));
    }

    /* Event not handled but subscribed. */
    __ASSERT_NO_MSG(false);

    return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, button_event);
