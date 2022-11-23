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

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/zephyr.h>

#include <string.h>

/* How much to wait between read polls. */
#ifndef UART_POLL_TIME_MS
#define UART_POLL_TIME_MS 100
#endif // UART_POLL_TIME_MS

/*
 * Wait for the uart device to connect and for uart to be ready.
 */
void uart_init(const struct device* uart_dev);

/*
 * Write the buffer to the output uart device.
 */
void uart_write(const struct device* uart_dev, const char* buf, size_t len);

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
bool uart_read_until(const struct device* uart_dev, char* buf, size_t len, char stop);

/*
 * Read from uart device until a newline character was read or the buffer was exhausted.
 * Returns false if the buffer was exhausted before a newline was found.
 */
#define uart_read_line(dev, buf, len) uart_read_until(dev, buf, len, '\n')

/*
 * Read from uart device until buffer is full.
 */
void uart_read(const struct device* uart_dev, char* buf, size_t len);
