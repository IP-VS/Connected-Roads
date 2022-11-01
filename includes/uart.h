#pragma once
/*
 * Copyright (c) 2022 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include <string.h>


#define MSG_SIZE 32

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

// static const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

/*
 * Wait for device to connect
 */
void uart_init(const struct device* uart_dev) {
    uint32_t dtr = 0;
    if (usb_enable(NULL)) {
        return;
    }
    //while (!dtr) {
        //uart_line_ctrl_get(uart_dev, UART_LINE_CTRL_DTR, &dtr);
        /* Give CPU resources to low priority threads. */
        k_sleep(K_MSEC(100));
    //}
	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!");
		return;
	}
	/* configure interrupt and callback to receive data */
	uart_irq_rx_enable(uart_dev);
}

/*
 * Writes the string "buf" to the output uart device.
 */
void uart_write(const struct device* uart_dev, const char* buf) {
    int msg_len = strlen(buf);
    for (int i = 0; i < msg_len; i++) {
        uart_poll_out(&uart_msgq, buf[i]);
    }
}

/*
 * Read from serial
 */
void uart_read(const struct device* uart_dev, char *buf)
{
    uart_poll_in(uart_dev, &buf);
}