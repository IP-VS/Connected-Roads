#include "microphone.h"
#include "printk.h"

#include <zephyr/drivers/i2s.h>

#define I2S_RX_NODE DT_NODELABEL(i2s_rx)

#define SAMPLE_FREQUENCY 32000
#define SAMPLE_BIT_WIDTH 24
#define BYTES_PER_SAMPLE sizeof(int32_t)
#define SAMPLES_PER_CH_PER_FRAME 128
#define NUMBER_OF_CHANNELS 2
#define SAMPLES_PER_FRAME (SAMPLES_PER_CH_PER_FRAME * NUMBER_OF_CHANNELS)
#define BYTES_PER_FRAME (SAMPLES_PER_FRAME * BYTES_PER_SAMPLE)
#define TIMEOUT 1000

K_MEM_SLAB_DEFINE_STATIC(mem_slab, BYTES_PER_FRAME, 2, 4);

static bool configure_stream(const struct device* i2s_dev_rx, const struct i2s_config* config) {
    int ret;

    ret = i2s_configure(i2s_dev_rx, I2S_DIR_RX, config);

    if (ret < 0) {
        printk("Failed to configure RX stream: %d\n", ret);
        return false;
    }

    return true;
}

static bool trigger_command(const struct device* i2s_dev_rx, enum i2s_trigger_cmd cmd) {
    int ret;

    ret = i2s_trigger(i2s_dev_rx, I2S_DIR_RX, cmd);
    if (ret < 0) {
        printk("Failed to trigger command %d on RX: %d\n", cmd, ret);
        return false;
    }

    return true;
}

static void process_block_data(void* mem_block, uint32_t number_of_samples) {
    printk("a\n");
    k_mem_slab_free(&mem_slab, &mem_block);
    return;
}

void start_i2s_sampling(void) {
    const struct device* const i2s_dev_rx = DEVICE_DT_GET(I2S_RX_NODE);
    struct i2s_config config;

    if (!device_is_ready(i2s_dev_rx)) {
        printk("I2S device is not ready\n", i2s_dev_rx->name);
        return;
    }

    config.word_size = SAMPLE_BIT_WIDTH;
    config.channels = NUMBER_OF_CHANNELS;
    config.format = I2S_FMT_DATA_FORMAT_I2S;
    config.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER;
    config.frame_clk_freq = SAMPLE_FREQUENCY;
    config.mem_slab = &mem_slab;
    config.block_size = BYTES_PER_FRAME;
    config.timeout = TIMEOUT;

    if (!configure_stream(i2s_dev_rx, &config)) {
        return;
    }

    for (;;) {
        if (!trigger_command(i2s_dev_rx, I2S_TRIGGER_START)) {
            return;
        }

        while (1) {
            void* mem_block;
            uint32_t block_size;
            int ret;

            ret = i2s_read(i2s_dev_rx, &mem_block, &block_size);
            if (ret < 0) {
                printk("Failed to read data: %d\n", ret);
                break;
            }

            process_block_data(&mem_block, SAMPLES_PER_FRAME);
        }

        // if (!trigger_command(i2s_dev_rx, I2S_TRIGGER_STOP)) {
        //     return;
        // }
    }
}