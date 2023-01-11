
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "heartbeat.h"
#include "msgdata.h"

#include "printk.h"

static uint8_t dev_uuid[16];

static struct k_timer timer;
static void timer_callback(struct k_timer *timer) {
	// Send alive msg
    size_t n = snprintf(NULL, 0, "alive:UUID:%02x%02x", dev_uuid[0], dev_uuid[1]);
	char msg[n+1];
	(void)snprintf(msg, n+1, "alive:UUID:%02x%02x", dev_uuid[0], dev_uuid[1]);
	msg[n] = 0;
	if (bt_mesh_is_provisioned()) {
        (void)gen_msg_send(msg);
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