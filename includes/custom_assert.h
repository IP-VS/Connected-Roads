#pragma once

#include <stdbool.h>
#include <stdlib.h>

#define assert_not_null(ptr)                                                                                       \
    do {                                                                                                           \
        bool _val = ptr;                                                                                           \
        if (!_val) {                                                                                               \
            printk("Assertion failed: %s is null in %s:%d (%s), aborting.\n", #ptr, __FILE__, __LINE__, __func__); \
            abort();                                                                                               \
        }                                                                                                          \
    } while (0)
