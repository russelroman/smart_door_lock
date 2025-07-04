/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/gpio_pins.h>

/* This configuration file is included only once from button module and holds
 * information about pins forming keyboard matrix.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} buttons_def_include_once;

static const struct gpio_pin col[] = {
	{ .port = 1, .pin = DT_GPIO_PIN(DT_NODELABEL(col0), gpios) },
	{ .port = 1, .pin = DT_GPIO_PIN(DT_NODELABEL(col1), gpios) },
	{ .port = 1, .pin = DT_GPIO_PIN(DT_NODELABEL(col2), gpios) },
};

static const struct gpio_pin row[] = {
	{ .port = 1, .pin = DT_GPIO_PIN(DT_NODELABEL(row0), gpios) },
	{ .port = 1, .pin = DT_GPIO_PIN(DT_NODELABEL(row1), gpios) },
	{ .port = 1, .pin = DT_GPIO_PIN(DT_NODELABEL(row2), gpios) },
	{ .port = 1, .pin = DT_GPIO_PIN(DT_NODELABEL(row3), gpios) },
};
