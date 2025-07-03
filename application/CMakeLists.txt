#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/gatt_lock_svc.c
  src/gatt_dis_svc.c
  src/keypad.c
  src/gap_advertising.c
  src/gap_connection.c
  src/security.c
)

zephyr_include_directories(
  configuration/${NORMALIZED_BOARD_TARGET}
  )

# NORDIC SDK APP END
zephyr_library_include_directories(.)
