#pragma once

#include "datastructures.h"

#define NUM_OVERSAMPLING 10
#define OVERSAMPLING_PERIOD_US 100  // 10kHz
#define SAMPLING_PERIOD_MS 10       // 100Hz
#define NUM_SAMPLE_BUFFERS 5        // must be >= 2

typedef int16_t adc_value_t;

int start_adc_sampling();