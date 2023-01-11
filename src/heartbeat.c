#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "heartbeat.h"
#include "msgdata.h"

#include "printk.h"

static uint8_t dev_uuid[16];

static struct k_timer timer;
static void timer_callback(struct k_timer* timer) {
    char buf[sizeof(dev_uuid) * 2 + 1];
    // Add uuid to msg
    char* iter = buf;
    for (size_t i = 0; i < sizeof(dev_uuid); ++i) {
        sprintf(iter, "%02x", dev_uuid[i]);
        iter += 2;
    }
    buf[sizeof(buf) - 1] = 0;
    if (bt_mesh_is_provisioned()) {
        (void)gen_msg_send(MSG_HEARTBEAT, buf, sizeof(buf));
        return;
    }
}

void heartbeat_init(uint8_t sleeptime) {
    int err;
    if (IS_ENABLED(CONFIG_HWINFO)) {
        err = hwinfo_get_device_id(dev_uuid, sizeof(dev_uuid));
    }

    if (err < 0) {
        dev_uuid[0] = 0xdd;
        dev_uuid[1] = 0xdd;
    }
    k_timer_init(&timer, timer_callback, NULL);
    k_timer_start(&timer, K_SECONDS(sleeptime), K_SECONDS(sleeptime));
}
