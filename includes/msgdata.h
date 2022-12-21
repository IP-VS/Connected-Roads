#ifndef MSGDATA_H
#define MSGDATA_H

/* msgdata.h - msg data */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>

#include "board.h"
#include "printk.h"

/** Broadcasts a string to the mesh */
int gen_msg_send(char* val);

static uint8_t dev_uuid[16];
void msgdata_init(void);

#endif /* MSGDATA_H */
