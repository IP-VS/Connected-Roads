#include "ip_model.h"

#include "custom_assert.h"
#include "printk.h"
#include "provision.h"
#include <string.h>

bool samples_deserialize(const uint8_t* buf, size_t size, struct Samples* out_samples) {
    if (size < SAMPLES_SERIALIZE_BUFFER_SIZE) {
        printk("samples_deserialize: buffer too small to hold samples\r\n");
        return false;
    }
    assert_not_null(buf);
    assert_not_null(out_samples);
    memcpy(out_samples, buf, sizeof(struct Samples));
    return true;
}

bool samples_serialize(uint8_t* buf, size_t size, const struct Samples* out_samples) {
    if (size < SAMPLES_SERIALIZE_BUFFER_SIZE) {
        printk("samples_serialize: buffer too small to hold samples\r\n");
        return false;
    }
    assert_not_null(buf);
    assert_not_null(out_samples);
    memcpy(buf, out_samples, sizeof(struct Samples));
    return true;
}

// Simple ring-buffer implementation of a queue.
static struct Samples samples_send_q[SAMPLES_TO_SEND_QUEUE_LEN];
// must be locked before every access
K_MUTEX_DEFINE(samples_send_q_mtx);
// read&write pointers
static size_t samples_send_q_rp = 0;
static size_t samples_send_q_wp = 0;

bool enqueue_samples_to_send(const struct Samples* samples) {
    bool result = true;
    k_mutex_lock(&samples_send_q_mtx, K_FOREVER);

    assert_not_null(samples);

    {
        if ((samples_send_q_wp + 1) % sizeof(samples_send_q) == samples_send_q_rp) {
            printk("Error: Samples queue is full\r\n");
            result = false;
            goto end;
        }
        printk("Enqueueing samples, wp=%u, rp=%u\r\n", samples_send_q_wp, samples_send_q_rp);
        memcpy(&samples_send_q[samples_send_q_wp], samples, sizeof(struct Samples));
        samples_send_q_wp = (samples_send_q_wp + 1) % sizeof(samples_send_q);
        printk("Enqueued samples, wp=%u, rp=%u\r\n", samples_send_q_wp, samples_send_q_rp);
    }

end:
    k_mutex_unlock(&samples_send_q_mtx);
    return result;
}

bool dequeue_samples_to_send(struct Samples* out_samples) {
    bool result = true;
    k_mutex_lock(&samples_send_q_mtx, K_FOREVER);

    assert_not_null(out_samples);

    {
        if (samples_send_q_rp == samples_send_q_wp) {
            printk("Error: Samples queue is empty\r\n");
            result = false;
            goto end;
        }
        printk("Dequeueing samples, wp=%u, rp=%u\r\n", samples_send_q_wp, samples_send_q_rp);
        memcpy(out_samples, &samples_send_q[samples_send_q_rp], sizeof(struct Samples));
        samples_send_q_rp = (samples_send_q_rp + 1) % sizeof(samples_send_q);
        printk("Dequeued samples, wp=%u, rp=%u\r\n", samples_send_q_wp, samples_send_q_rp);
    }

end:
    k_mutex_unlock(&samples_send_q_mtx);
    return result;
}

static void send_start(uint16_t duration, int err, void* cb_data) {
    printk("send start, duration=%d, err=%d, addr=%d\r\n", duration, err, (uint16_t)cb_data);
}

static void send_end(int err, void* cb_data) {
    printk("send end, err=%d, addr=%d\r\n", err, (uint16_t)cb_data);
}

static struct bt_mesh_send_cb send_cb = {
    .start = send_start,
    .end = send_end,
};

int handle_message_micdata(struct bt_mesh_model* model, struct bt_mesh_msg_ctx* ctx, struct net_buf_simple* buf) {
    // Message handler code
    printk("Got MICDATA message\r\n");
    return 0;
}

int send_message(struct bt_mesh_model* model, uint16_t addr) {
    struct bt_mesh_msg_ctx ctx = {
        .net_idx = 0,
        .addr = addr,
        .app_idx = model->keys[0],
        .send_ttl = BT_MESH_TTL_DEFAULT,
        .send_rel = true,
    };

    BT_MESH_MODEL_BUF_DEFINE(buf, MESSAGE_MICDATA_OPCODE, MESSAGE_MICDATA_LEN);
    bt_mesh_model_msg_init(&buf, MESSAGE_MICDATA_OPCODE);

    printk("Sending MICDATA message to 0x%04x\r\n", addr);

    const char mem[] = "hello";

    net_buf_simple_add_mem(&buf, mem, sizeof(mem));

    return bt_mesh_model_send(model, &ctx, &buf, &send_cb, (void*)addr);
}

bool send_micdata_from_queue(uint16_t addr) {
    struct Samples samples;
    if (!dequeue_samples_to_send(&samples)) {
        return false;
    }

    struct bt_mesh_model* model = get_msg_model();
    if (!model) {
        printk("Error: Message model is NULL\r\n");
        return false;
    }

    // TODO: do for all addresses, not just one.
    struct bt_mesh_msg_ctx ctx = {
        .addr = addr,
        .app_idx = model->keys[0],
        .send_ttl = BT_MESH_TTL_DEFAULT,
        .send_rel = true,
    };

    BT_MESH_MODEL_BUF_DEFINE(buf, MESSAGE_MICDATA_OPCODE, MESSAGE_MICDATA_LEN);
    bt_mesh_model_msg_init(&buf, MESSAGE_MICDATA_OPCODE);

    // serialize samples into message buffer
    if (!samples_serialize(buf.data, MESSAGE_MICDATA_LEN, &samples)) {
        return false;
    }
    buf.len = SAMPLES_SERIALIZE_BUFFER_SIZE;

    printk("Sending MICDATA message to 0x%04x with app_idx 0x%04x\r\n", addr, model->keys[0]);
    return bt_mesh_model_send(model, &ctx, &buf, &send_cb, (void*)addr);
}
