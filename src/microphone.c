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
static adc_value_t adc_bufs[NUM_CHANNELS][NUM_OVERSAMPLING];

static struct adc_sequence_options adc_options[NUM_CHANNELS];
static struct adc_sequence adc_sequences[NUM_CHANNELS];

struct k_timer adc_timer;

static void adc_work_handler(struct k_work *work)
{
    int err;
    for (size_t i = 0; i < NUM_CHANNELS; i++)
    {
        size_t channel_idx = adc_channel_idxs[i];
        (void) adc_sequence_init_dt(&adc_channels[channel_idx], &adc_sequences[i]);
        err = adc_read(adc_channels[channel_idx].dev, &adc_sequences[i]);

        // if (err < 0) {
        //     printk("adc error (%d) channel %d\n", err, i);
        //     continue;
        // }

        printk("channel %d: ", i);
        for (size_t k = 0; k < NUM_OVERSAMPLING; k++)
        {
            printk("%d,", adc_bufs[i][k]);
        }
    }
}

K_WORK_DEFINE(adc_work, adc_work_handler);

static void adc_timer_handler(struct k_timer *dummy)
{
    k_work_submit(&adc_work);
}

K_TIMER_DEFINE(adc_timer, adc_timer_handler, NULL);

int start_adc_sampling()
{
    int err;
    for (size_t i = 0; i < NUM_CHANNELS; i++)
    {
        size_t channel_idx = adc_channel_idxs[i];

        adc_options[i].interval_us = OVERSAMPLING_PERIOD_US;
        adc_options[i].extra_samplings = NUM_OVERSAMPLING - 1;
        adc_options[i].callback = NULL;

        adc_sequences[i].buffer = &adc_bufs[i];
        adc_sequences[i].buffer_size = NUM_OVERSAMPLING * sizeof(adc_value_t);
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

    k_timer_init(&adc_timer, adc_timer_handler, NULL);
    k_timer_start(&adc_timer, K_SECONDS(0), K_MSEC(SAMPLING_PERIOD_MS));

    return 0;
}