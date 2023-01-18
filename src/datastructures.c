#include "datastructures.h"

#include "custom_assert.h"
#include "printk.h"
#include <stdint.h>
#include <string.h>

bool samples_deserialize(const uint8_t* buf, size_t size, struct Samples* out_samples) {
    if (size < SAMPLES_SERIALIZE_BUFFER_SIZE) {
        printk("samples_deserialize: buffer too small to hold samples\r\n");
        return false;
    }
    assert_not_null(buf);
    assert_not_null(out_samples);
    memcpy(out_samples, buf, sizeof(struct Samples));
    return true;
}

bool samples_serialize(uint8_t* buf, size_t size, const struct Samples* out_samples) {
    if (size < SAMPLES_SERIALIZE_BUFFER_SIZE) {
        printk("samples_serialize: buffer too small to hold samples\r\n");
        return false;
    }
    assert_not_null(buf);
    assert_not_null(out_samples);
    memcpy(buf, out_samples, sizeof(struct Samples));
    return true;
}

