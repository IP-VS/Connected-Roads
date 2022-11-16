/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh/main.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/zephyr.h>

#include "provision.h"

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart),
    "Console device is not ACM CDC UART device");

static const struct bt_mesh_comp node_comp = {
    .cid = CONFIG_BT_COMPANY_ID,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

static const struct bt_mesh_prov node_prov = {
    .uuid = dev_uuid,
};

static int run_bt_node(void) {
    uint8_t net_key[16], dev_key[16];
    int err;

    err = bt_mesh_init(&node_prov, &node_comp);
    if (err) {
        printk("Initializing mesh failed (err %d)\n", err);
        return;
    }

    printk("Mesh initialized\n");

    if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
        printk("Loading stored settings\n");
        settings_load();
    }

    err = bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);
    if (err) {
        printk("bt_mesh_prov_enable failed (err %d)\n", err);
    } else {
        printk("bt_mesh_prov_enable ok\n");
    }

    return 0;
}

void main(void) {
    /* uart */
    uart_init(dev);

    int err;

    button_init();

    printk("Press button 1 within 5 seconds to make this node a provisioner\n");
    if (wait_for_button_press(5)) {
        provision();
        printk("Done provisioning\n");
    } else {
        err = bt_enable(NULL);
        if (err) {
            printk("Bluetooth init failed (err %d)\n", err);
            return;
        }
        run_bt_node();
    }

    printk("Main reached end :)\n");

    while (1) {
        k_sleep(K_SECONDS(1));
        if (bt_mesh_is_provisioned()) {
            printk("Node is provisioned!\n");
            break;
        }
    }
    printk("Ready to do work!\n");
    while (1) {
        k_sleep(K_SECONDS(1));
    }
}
