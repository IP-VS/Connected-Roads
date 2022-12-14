#include "microphone.h"
#include "ip_model.h"
#include "printk.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>

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

static struct Samples sample_bufs[NUM_SAMPLE_BUFFERS];
static size_t current_sample_buf = 0;
static size_t previous_sample_buf = -1;

struct k_timer adc_timer;

static int32_t postproc_first(adc_value_t *buf)
{
    return buf[0];
}

static int32_t postproc_sum(adc_value_t *buf)
{
    int32_t sum = 0;
    for (size_t i = 0; i < NUM_OVERSAMPLING; i++)
    {
        sum += buf[i];
    }
    return sum;
}

static void adc_work_handler(struct k_work *work)
{
    struct Samples *samples = &sample_bufs[current_sample_buf];

    if (previous_sample_buf != current_sample_buf && samples->n_samples != 0)
    {
        // message has not been sent yet
        return;
    }

    previous_sample_buf = current_sample_buf;

    int err = 0;
    int64_t time = k_uptime_get();

    for (size_t i = 0; i < NUM_CHANNELS; i++)
    {
        size_t channel_idx = adc_channel_idxs[i];
        (void)adc_sequence_init_dt(&adc_channels[channel_idx], &adc_sequences[i]);

        // a measurement has failed when sampling on any channel failed
        err = err != 0 ? err : adc_read(adc_channels[channel_idx].dev, &adc_sequences[i]);

        samples->samples[samples->n_samples].channels[i] = postproc_first(adc_bufs[i]);
    }

    if (err == 0)
    {
        samples->samples[samples->n_samples].time = time;
        samples->n_samples++;
    }
    else
    {
        printk("adc error (%d)\n", err);
        return;
    }

    if (samples->n_samples >= NUM_SAMPLES)
    {
        enqueue_samples_to_send(samples);

        if (!enqueue_samples_to_send(samples))
        {
            printk("failed to enqueue samples\n");
            // reuse same buffer
            samples->n_samples = 0;
        }
        else
        {
            current_sample_buf++;
            if (current_sample_buf >= NUM_SAMPLE_BUFFERS)
            {
                current_sample_buf = 0;
            }
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

    for (size_t i = 0; i < NUM_SAMPLE_BUFFERS; i++)
    {
        sample_bufs[i].n_samples = 0;
    }

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