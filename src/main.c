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

#include "ip_model.h"
#include "provision.h"

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart),
    "Console device is not ACM CDC UART device");

void main(void) {
    /* uart */
    uart_init(dev);

    printk("Startup\r\n");

    int err;

    dev_uuid_init();

    button_init();

    printk("Press button 1 within 5 seconds to make this node a provisioner\r\n");
    if (wait_for_button_press(5)) {
        provision();
        printk("Done provisioning\r\n");
    } else {
        err = bt_enable(NULL);
        if (err) {
            printk("Bluetooth init failed (err %d)\r\n", err);
            return;
        }
        run_bt_node();
    }

    printk("Main reached end :)\r\n");

    while (1) {
        k_sleep(K_SECONDS(1));
        if (bt_mesh_is_provisioned()) {
            printk("Node is provisioned!\r\n");
            break;
        }
    }
    printk("Ready to do work!\r\n");
    while (1) {
        // TODO: Adjust address here if you want to test
        if (!send_micdata_from_queue(0x01)) {
            k_sleep(K_SECONDS(1));
        }
    }
}
