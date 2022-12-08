#pragma once

#include "datastructures.h"

#define NUM_OVERSAMPLING 5
#define OVERSAMPLING_PERIOD_US 100
#define SAMPLING_PERIOD_MS 1000

typedef int16_t adc_value_t;

int start_adc_sampling();