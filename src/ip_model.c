#include "ip_model.h"

#include "printk.h"

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
    printk("Got STATUS message\r\n");
    return 0;
}

int send_message(struct bt_mesh_model* model, uint16_t addr) {
    struct bt_mesh_msg_ctx ctx = {
        .addr = addr,
        .app_idx = model->keys[0],
        .send_ttl = BT_MESH_TTL_DEFAULT,
        .send_rel = true,
    };

    BT_MESH_MODEL_BUF_DEFINE(buf, MESSAGE_MICDATA_OPCODE, MESSAGE_MICDATA_LEN);
    bt_mesh_model_msg_init(&buf, MESSAGE_MICDATA_OPCODE);

    printk("Sending MICDATA message to 0x%04x\r\n", addr);

    buf.data = "hello";

    return bt_mesh_model_send(model, &ctx, &buf, &send_cb, (void*)addr);
}
