#include "microphone.h"
#include "printk.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>

BUILD_ASSERT(DT_NODE_EXISTS(DT_PATH(zephyr_user)) && DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels),
             "No suitable device for ADC");

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
    ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

static const struct adc_dt_spec adc_channels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
                         DT_SPEC_AND_COMMA)};

static size_t adc_channel_idxs[NUM_CHANNELS] = {0, 1};
static int16_t adc_bufs[NUM_CHANNELS][NUM_SAMPLES_PER_TRIGGER];

static struct adc_sequence_options adc_options[NUM_CHANNELS];
static struct adc_sequence adc_sequences[NUM_CHANNELS];

int init_adc()
{
    int err;
    for (size_t i = 0; i < NUM_CHANNELS; i++)
    {
        size_t channel_idx = adc_channel_idxs[i];

        adc_options[i].interval_us = OVERSAMPLING_PERIOD_US;
        adc_options[i].extra_samplings = NUM_SAMPLES_PER_TRIGGER - 1;

        adc_sequences[i].buffer = &adc_bufs[i];
        adc_sequences[i].buffer_size = NUM_SAMPLES_PER_TRIGGER * sizeof(int16_t);
        adc_sequences[i].options = &adc_options[i];

        if (!device_is_ready(adc_channels[channel_idx].dev))
        {
            printk("ADC controller device not ready #%d\n", channel_idx);
            return -1;
        }

        err = adc_channel_setup_dt(&adc_channels[channel_idx]);
        if (err < 0)
        {
            printk("Could not setup channel #%d (%d)\n", channel_idx, err);
            return -2;
        }
    }

    return 0;
}