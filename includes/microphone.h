#pragma once

/*
 * Temporary microphone API for raw data to see in webserver
 * This uses the synchronous ADC API
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/zephyr.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include "printk.h"

enum adc_action adc_callback(const struct device *dev, const struct adc_sequence *sequence, uint16_t sampling_index);

int init_adc();

void adc_work_handler(struct k_work *work);

void adc_timer_handler(struct k_timer *dummy);

void microphone_init(void);
