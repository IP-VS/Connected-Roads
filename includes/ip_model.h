#pragma once

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/mesh.h>

#define COMPANY_ID 0xcafe
#define MODEL_ID 0xdad0

#define MESSAGE_MICDATA_OPCODE BT_MESH_MODEL_OP_3(0x01, COMPANY_ID)

#define MESSAGE_MICDATA_LEN 64

int handle_message_micdata(struct bt_mesh_model* model,
    struct bt_mesh_msg_ctx* ctx,
    struct net_buf_simple* buf);

int send_message(struct bt_mesh_model* model, uint16_t addr);
