#pragma once

/*
 * This header-only API contains helpers for reading and writing arbitrary data
 * from / to a uart device. It's a blocking, polling API, and can be used for strings
 * (null-terminated) as well as binary data.
 * 
 * In all cases, you need to *make sure* that your buffer is zeroed out before
 * you give it to any of the functions, if that's important to your use-case.
 * 
 * == Quickstart ==
 * 
 * - Writing strings:
 * 
 *   void uart_write_str(dev, "my string");
 * 
 * - Writing binary data:
 * 
 *   void uart_write(dev, mydata, mydata_size);
 * 
 * - Reading until a specific byte (here for example newline):
 * 
 *   char delim = '\n';
 *   if (!uart_read_until(dev, buffer, buffer_size, delim)) {
 *       // buffer exhausted, handle this
 *   }
 *  
 *   in this case, you need to handle the case that the buffer ran out of 
 *   space before the specified byte was read.
 * 
 * - Reading a specific number of bytes:
 * 
 *   uart_read(dev, buffer, buffer_size);
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include <string.h>

/* How much to wait between read polls. */
#ifndef UART_POLL_TIME_MS
#define UART_POLL_TIME_MS 100
#endif // UART_POLL_TIME_MS

/*
 * Wait for the uart device to connect and for uart to be ready.
 */
static inline void uart_init(const struct device* uart_dev) {
    uint32_t dtr = 0;
    if (usb_enable(NULL)) {
        return;
    }
    while (!dtr) {
        uart_line_ctrl_get(uart_dev, UART_LINE_CTRL_DTR, &dtr);
        /* Give CPU resources to low priority threads. */
        k_sleep(K_MSEC(UART_POLL_TIME_MS));
    }
	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!");
		return;
	}
	/* configure interrupt and callback to receive data */
	uart_irq_rx_enable(uart_dev);
}

/*
 * Write the buffer to the output uart device.
 */
static inline void uart_write(const struct device* uart_dev, const char* buf, size_t len) {
    for (int i = 0; i < len; i++) {
        uart_poll_out(uart_dev, buf[i]);
    }
}

/*
 * Write the string to the output uart device.
 */
#define uart_write_str(dev, str) uart_write(dev, str, strlen(str))

/*
 * Read from uart device until a specific character was read,
 * or the buffer is full. 
 * Returns false if the buffer was exhausted before reading the
 * specified character.
 */
static inline bool uart_read_until(const struct device* uart_dev, char* buf, size_t len, char stop)
{
    size_t i = 0;
    do {
        if (i > len) {
            return false;
        }
        while (uart_poll_in(uart_dev, &buf[i]) < 0)
            k_sleep(K_MSEC(UART_POLL_TIME_MS));
        ++i;
    } while (buf[i-1] != stop);
    return true;
}

/*
 * Read from uart device until a newline character was read or the buffer was exhausted.
 * Returns false if the buffer was exhausted before a newline was found.
 */
#define uart_read_line(dev, buf, len) uart_read_until(dev, buf, len, '\n')

/*
 * Read from uart device until buffer is full.
 */
static inline void uart_read(const struct device* uart_dev, char* buf, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        while (uart_poll_in(uart_dev, &buf[i]) < 0)
            k_sleep(K_MSEC(UART_POLL_TIME_MS));
    }
}
