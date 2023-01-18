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

#include "board.h"
#include "heartbeat.h"
#include "microphone.h"
#include "msgdata.h"
#include "provision.h"

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart),
    "Console device is not ACM CDC UART device");

void main(void) {
    /* uart */
    uart_init(dev);
    dev_uuid_init();

    start_i2s_sampling();

    // printk("Startup \r\n");

    // printk("Press button 1 within 5 seconds to make this node a provisioner\r\n");
    // if (wait_for_button_press(5)) {
    //     provision();
    //     printk("Done provisioning\r\n");
    // } else {
    //     msgdata_init();
    //     // Heartbeat init AFTER msgdata init
    //     heartbeat_init(5);
    // }

    // printk("Main reached end :)\r\n");

    // // Mic stuff
    // // while (1) {
    // //     k_sleep(K_SECONDS(1));
    // //     if (bt_mesh_is_provisioned()) {
    // //         printk("Node is provisioned!\r\n");
    // //         printk("Mic Init \r\n");
    // //         microphone_init();
    // //         break;
    // //     }
    // // }
    // // printk("Ready to do work!\r\n");
    // // while (1) {
    // //     k_sleep(K_SECONDS(1));
    // // }

    printk("done\n");
    while (1) {
        k_sleep(K_SECONDS(1));
    }
}
