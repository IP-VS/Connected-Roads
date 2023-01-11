#ifndef PRINTK_H
#define PRINTK_H

/*
 * Overrides "printk()" by #define-ing it to instead call
 * uart_write.
 * If it doesn't work, make sure the #include "printk.h" is
 * the LAST include in your file, and that the device `dev`
 * has been properly initialized.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/settings/settings.h>

#include "uart.h"

static const struct device* dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

// check for VLA support
#ifdef __STDC_NO_VLA__
#error "VLA NOT SUPPORTED"
#endif

#define printk_raw(...)                            \
    do {                                           \
        int _size = snprintf(NULL, 0, __VA_ARGS__); \
        char _buf[_size + 1];                        \
        snprintf(_buf, sizeof(_buf), __VA_ARGS__);   \
        uart_write(dev, _buf, sizeof(_buf));         \
    } while (false)

// override printk
#define printk(...)                                \
    do {                                           \
        printk_raw("%s:%d: ", __FILE__, __LINE__); \
        printk_raw(__VA_ARGS__);                   \
    } while (false)

#endif // PRINTK_H
