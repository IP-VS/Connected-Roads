#pragma once

#include <stdbool.h>
#include <stdlib.h>

#define assert_not_null(ptr)                                                          \
    do {                                                                              \
        bool _val = (ptr == NULL);                                                    \
        if (_val) {                                                                   \
            printk("Assertion failed: %s is null in %s:%d (%s), aborting.\r\n", #ptr, \
                __FILE__, __LINE__, __func__);                                        \
            abort();                                                                  \
        }                                                                             \
    } while (0)

#define assert_equal(left, right)                                                                     \
    do {                                                                                              \
        bool _val = ((left) == (right));                                                              \
        if (!_val) {                                                                                  \
            printk("Assertion failed: %s == %s is false in %s:%d (%s), aborting.\r\n", #left, #right, \
                __FILE__, __LINE__, __func__);                                                        \
            abort();                                                                                  \
        }                                                                                             \
    } while (0)
