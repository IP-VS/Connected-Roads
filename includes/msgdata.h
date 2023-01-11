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

#include <zephyr/zephyr.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>

#include "board.h"
#include "printk.h"

/* Receiver */
static unsigned int recv_addr = BT_MESH_ADDR_ALL_NODES;

/* Listen to UART */
void send_msg_from_uart();

/* Generic OnOff Client */
static int gen_msg_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf);

/* Provisioning */
static int output_number(bt_mesh_output_action_t action, uint32_t number);

/** Broadcasts a string to the mesh */
int gen_msg_send(char *val);

static uint8_t dev_uuid[16];
void msgdata_init(struct device *uart_dev);

void test_init(uint8_t sleeptime);
#endif /* MSGDATA_H */