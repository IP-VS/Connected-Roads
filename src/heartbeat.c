
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
    char* msg[50]; // Dynamically resizing does not work
    strcpy(msg, "alive:UUID:");
    // Add uuid to msg
    sprintf(msg, "%s%02x%02x", msg, dev_uuid[0], dev_uuid[1]);
	if (bt_mesh_is_provisioned()) {
        (void)gen_msg_send(msg);
        free(msg);
		return;
	}
    free(msg);
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