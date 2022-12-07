#include "network.h"
#include "printk.h"

#include <string.h>

bool samples_deserialize(const uint8_t* buf, size_t size, struct Samples* out_samples) {
    if (size < SAMPLES_SERIALIZE_BUFFER_SIZE) {
        printk("samples_deserialize: buffer too small to hold samples\r\n");
        return false;
    }
    memcpy(out_samples, buf, sizeof(struct Samples));
    return true;
}

bool samples_serialize(uint8_t* buf, size_t size, const struct Samples* out_samples) {
    if (size < SAMPLES_SERIALIZE_BUFFER_SIZE) {
        printk("samples_serialize: buffer too small to hold samples\r\n");
        return false;
    }
    memcpy(buf, out_samples, sizeof(struct Samples));
    return true;
}

// Simple ring-buffer implementation of a queue.
static struct Samples samples_send_q[SAMPLES_TO_SEND_QUEUE_LEN];
// must be locked before every access
static struct k_mutex samples_send_q_mtx;
// read&write pointers
static size_t samples_send_q_rp = 0;
static size_t samples_send_q_wp = 0;

bool enqueue_samples_to_send(const struct Samples* samples) {
    bool result = true;
    k_mutex_lock(&samples_send_q_mtx, K_FOREVER);

    {
        if ((samples_send_q_wp + 1) % sizeof(samples_send_q) == samples_send_q_rp) {
            printk("Error: Samples-to-send queue is full\r\n");
            result = false;
            goto end;
        }
        memcpy(&samples_send_q[samples_send_q_wp], samples, sizeof(struct Samples));
        samples_send_q_wp = (samples_send_q_wp + 1) % sizeof(samples_send_q);
    }

end:
    k_mutex_unlock(&samples_send_q_mtx);
    return result;
}

bool dequeue_samples_to_send(struct Samples* out_samples) {
    bool result = true;
    k_mutex_lock(&samples_send_q_mtx, K_FOREVER);

    {
        if (samples_send_q_rp == samples_send_q_wp) {
            result = false;
            goto end;
        }
        memcpy(out_samples, &samples_send_q[samples_send_q_rp], sizeof(struct Samples));
        samples_send_q_rp = (samples_send_q_rp + 1) % sizeof(samples_send_q);
    }

end:
    k_mutex_unlock(&samples_send_q_mtx);
    return result;
}
