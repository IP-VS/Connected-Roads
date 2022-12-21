/*
 * Copyright (c) 2019 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PROVISION_H
#define PROVISION_H
#include <assert.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/settings/settings.h>

#include "printk.h"

#define SW0_NODE DT_ALIAS(sw0)

void health_current_status(struct bt_mesh_health_cli* cli, uint16_t addr,
    uint8_t test_id, uint16_t cid, uint8_t* faults, size_t fault_count);

void setup_cdb(void);

void configure_self(struct bt_mesh_cdb_node* self);

void configure_node(struct bt_mesh_cdb_node* node);

void unprovisioned_beacon(uint8_t uuid[16],
    bt_mesh_prov_oob_info_t oob_info,
    uint32_t* uri_hash);

void node_added(uint16_t net_idx, uint8_t uuid[16], uint16_t addr, uint8_t num_elem);

int bt_ready(void);

uint8_t check_unconfigured(struct bt_mesh_cdb_node* node, void* data);

void button_pressed(const struct device* dev, struct gpio_callback* cb, uint32_t pins);

void button_init(void);

bool wait_for_button_press(unsigned timeout_s);

void provision(void);

int run_bt_node(void);

void dev_uuid_init(void);

#endif /* PROVISION_H */
