#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// number of channels
#define NUM_CHANNELS 2
// max samples per channel
#define NUM_SAMPLES 16

struct Sample {
    int64_t time;
    int32_t channels[NUM_CHANNELS];
};

struct Samples {
    size_t n_samples;
    struct Sample samples[NUM_SAMPLES];
};

#define SAMPLES_SERIALIZE_BUFFER_SIZE (sizeof(struct Samples))

bool samples_deserialize(const uint8_t* buf, size_t size, struct Samples* out_samples);
bool samples_serialize(uint8_t* buf, size_t size, const struct Samples* out_samples);
