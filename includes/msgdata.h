#ifndef MSGDATA_H
#define MSGDATA_H

/* msgdata.h - msg data */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/settings/settings.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>

#include "board.h"
#include "printk.h"

/* Generic OnOff Client */
static int gen_msg_status(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf);

/* Provisioning */

static int output_number(bt_mesh_output_action_t action, uint32_t number);

/** Send an OnOff Set message from the Generic OnOff Client to all nodes. */
static int gen_msg_send(bool val);

static uint8_t dev_uuid[16];
void msgdata_init(void);

#endif /* MSGDATA_H */