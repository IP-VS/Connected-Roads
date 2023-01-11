#include "microphone.h"

// ADC configuration
BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart),
	     "Console device is not ACM CDC UART device");

BUILD_ASSERT(DT_NODE_EXISTS(DT_PATH(zephyr_user)) &&
		     DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels),
	     "No suitable device for ADC");

#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = { DT_FOREACH_PROP_ELEM(
	DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA) };

void microphone_init(void)
{
	int err;
	// int16_t buf;
	int16_t buf2[256];
	int16_t marker[5] = { 0, 0, 0, 0, 0 };
	struct adc_sequence_options sequence_options = {
		.interval_us = 100, // 10kHz
		.extra_samplings = sizeof(buf2) / sizeof(int16_t) - 1,
	};
	struct adc_sequence sequence = {
		.buffer = &buf2,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf2),
		.options = &sequence_options,
	};
	/* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!device_is_ready(adc_channels[i].dev)) {
			printk("ADC controller device not ready\n");
			return;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			printk("Could not setup channel #%d (%d)\n", i, err);
			return;
		}
	}

	while (1) {
		for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
			int32_t val_mv;

			(void)adc_sequence_init_dt(&adc_channels[i], &sequence);

			err = adc_read(adc_channels[i].dev, &sequence);
			if (err < 0) {
				// printk("Could not read (%d)\n", err);
				continue;
			} else {
				printk_raw("micout: [\n"); // why is this not printing?
				for (size_t s = 0U; s < ARRAY_SIZE(buf2); s++) {
					if (s == ARRAY_SIZE(buf2) - 1) {
						printk_raw("%" PRId16 "", buf2[s]);
					} else {
						printk_raw("%" PRId16 ",", buf2[s]);
					}
				}
				printk_raw(" ]\n");
				k_sleep(K_SECONDS(1));

				// uart_write(dev, marker, sizeof(marker));
				// uart_write(dev, buf2, sizeof(buf2));
			}
		}
	}
}
