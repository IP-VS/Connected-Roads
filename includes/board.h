/* board.h - Board-specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <bluetooth/mesh/main.h>
#include <stdbool.h>
#include <stdint.h>

int board_init(struct k_work *button_work);

void board_led_set(bool val);
