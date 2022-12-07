#pragma once

/*
 * API for sending application-specific data over the network.
 */

#include "datastructures.h"
#include <stdbool.h>

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

