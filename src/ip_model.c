#include "ip_model.h"

#include "printk.h"

int handle_message_set(struct bt_mesh_model* model,
    struct bt_mesh_msg_ctx* ctx,
    struct net_buf_simple* buf) {
    // Message handler code
    printk("Got SET message\r\n");
}

int handle_message_ack(struct bt_mesh_model* model,
    struct bt_mesh_msg_ctx* ctx,
    struct net_buf_simple* buf) {
    // Message handler code
    printk("Got ACK message\r\n");
}

int handle_message_status(struct bt_mesh_model* model,
    struct bt_mesh_msg_ctx* ctx,
    struct net_buf_simple* buf) {
    // Message handler code
    printk("Got STATUS message\r\n");
}

int send_message(struct bt_mesh_model* model, uint16_t addr) {
    struct bt_mesh_msg_ctx ctx = {
        .addr = addr,
        .app_idx = model->keys[0],
        .send_ttl = BT_MESH_TTL_DEFAULT,
    };

    BT_MESH_MODEL_BUF_DEFINE(buf, MESSAGE_SET_OPCODE, MESSAGE_SET_LEN);
    bt_mesh_model_msg_init(&buf, MESSAGE_SET_OPCODE);

    printk("Sending SET message to 0x%04x\r\n", addr);

    buf.data = "hello";

    return bt_mesh_model_send(model, &ctx, &buf, NULL, NULL);
}
