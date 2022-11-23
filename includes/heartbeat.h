#ifndef heartbeat_h
#define heartbeat_h

/**
 * hearbeat client
 * via Periodic publication of heartbeat messages
 *
 */

#include <zephyr/zephyr.h>
#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>

#include <heartbeat.h>

// Semaphore for heartbeat
struct k_sem sem;

// Hearbeat parameters
#define WAIT_TIME 60 /*seconds*/
#define SUBSCRIBER_ADDR 0x0001
#define SUBSCRIBE_PERIOD_SEC 30
#define PUBLISHER_ADDR  0x000f
#define PUBLISH_PERIOD_SEC 1

#define PUBLISH_MSG_CNT 10
#define PUBLISH_TTL 0
#define EXPECTED_HB_HOPS 0x01

// Define callback structure as heartbeat cb
BT_MESH_HB_CB_DEFINE(hb_cb);

// Heartbeat callback receive function
static void heartbeat_recv_cb(const struct bt_mesh_hb_sub *sub, uint8_t hops, uint16_t feat)
{
	printk("Heartbeat from 0x%04x, hops %u, feat 0x%04x", sub->src, hops, feat);
}

// Heartbeat callback publish function
static void heartbeat_pub_cb(const struct bt_mesh_hb_pub *pub)
{
	printk("Heartbeat publication sent");
}

// Hearbeat Callback structure
static const struct bt_mesh_hb_cb hb_cb = {
	.recv = heartbeat_recv_cb,
	.pub_sent = heartbeat_pub_cb,
	// .sub_end = heartbeat_pub,
};

// Heartbeat Publication parameters (every 10 sec)
static const struct bt_mesh_hb_pub hb_pub = {
	.dst = BT_MESH_ADDR_UNASSIGNED,
	.count = 1,
	.ttl = 5,
	.feat = BT_MESH_FEAT_RELAY,
	.net_idx = BT_MESH_KEY_UNUSED,
	.period = BT_MESH_PUB_PERIOD_SEC(5),
};

// Heartbeat Subscription parameters
static struct bt_mesh_hb_sub hb_sub = {
	.src = BT_MESH_ADDR_UNASSIGNED,
	.dst = SUB,
	.min_hops = 1,
	.max_hops = 5,
	.period = BT_MESH_PUB_PERIOD_SEC(5),
};

// Heartbeat init function
void heartbeat_init(void)
{
	k_sem_init(&sem, 0, 1);

	int err;
	
	// Set heartbeat publication parameters
	err = bt_mesh_hb_pub_set(&hb_pub);
	if (err)
	{
		printk("Failed to set heartbeat publication (err %d)", err);
		return;
	}

	// Set heartbeat subscription parameters
	err = bt_mesh_hb_sub_set(&hb_sub);
	if (err)
	{
		printk("Failed to set heartbeat subscription (err %d)", err);
		return;
	}

	/* +1 to avoid boundary time rally */
	if (k_sem_take(&sem, K_SECONDS(PUBLISH_PERIOD_SEC * (PUBLISH_MSG_CNT + 1))))
	{
		printk("Publishing timed out");
		return;
	}
}

#endif