#pragma once

#include "datastructures.h"
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/mesh.h>

#define SAMPLES_SERIALIZE_BUFFER_SIZE (sizeof(struct Samples))

bool samples_deserialize(const uint8_t* buf, size_t size, struct Samples* out_samples);
bool samples_serialize(uint8_t* buf, size_t size, const struct Samples* out_samples);

// The size of the samples-to-send queue.
// Must be at least 1.
#define SAMPLES_TO_SEND_QUEUE_LEN 3

// Enqueues samples to be sent. Does NOT take ownership of `samples`,
// instead takes a copy of it. You may reuse `samples` after this call.
// This call is not blocking. Returns `false` if somehow the queue is full.
bool enqueue_samples_to_send(const struct Samples* samples);

// Dequeues the samples to be send. Returns `false` if the queue is empty.
bool dequeue_samples_to_send(struct Samples* out_samples);

#define COMPANY_ID 0xcafe
#define MODEL_ID 0xdad0

#define MESSAGE_MICDATA_OPCODE BT_MESH_MODEL_OP_3(0x01, COMPANY_ID)

#define MESSAGE_MICDATA_LEN SAMPLES_SERIALIZE_BUFFER_SIZE

int handle_message_micdata(struct bt_mesh_model* model,
    struct bt_mesh_msg_ctx* ctx,
    struct net_buf_simple* buf);

int send_message(struct bt_mesh_model* model, uint16_t addr);

// Checks the samples queue for new data, sends it if available.
// Returns `true` if data was found to be sent, `false` if no data was sent.
bool send_micdata_from_queue(uint16_t addr);
