#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include <unistd.h>

#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/mesh.h>


#define MOD_LF 0x0000

#define GROUP_ADDR 0xc000
#define PUBLISHER_ADDR  0x000f

#define OP_VENDOR_BUTTON BT_MESH_MODEL_OP_3(0x00, BT_COMP_ID_LF)

#if !defined(NODE_ADDR)
#define NODE_ADDR 0x0b0c
#endif

static const uint8_t net_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const uint8_t dev_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const uint8_t app_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const uint16_t net_idx;
static const uint16_t app_idx;
static const uint32_t iv_index;
static uint8_t flags;
static uint16_t addr = NODE_ADDR;

static const uint8_t dev_uuid[16] = { 0xdd, 0xdd };

static const struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
};

static K_SEM_DEFINE(tune_sem, 0, 1);
static const char *tune_str;

static void mesh_init(int err, const struct bt_mesh_comp *comp) {
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_mesh_init(&prov, comp);
	if (err) {
		printk("Initializing mesh failed (err %d)\n", err);
		return;
	}

	printk("Mesh initialized\n");

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		printk("Loading stored settings\n");
		settings_load();
	}

	err = bt_mesh_provision(net_key, net_idx, flags, iv_index, addr,
				dev_key);
	if (err == -EALREADY) {
		printk("Using stored settings\n");
	} else if (err) {
		printk("Provisioning failed (err %d)\n", err);
		return;
	} else {
		printk("Provisioning completed\n");
		// Continue to configure
	}


	printk("Configuring...\n");

	// Broadcast
	

	/* Add Application Key */
	// bt_mesh_cfg_cli_app_key_add(net_idx, addr, net_idx, app_idx, app_key, NULL);

	// /* Bind to vendor model */
	// bt_mesh_cfg_cli_mod_app_bind_vnd(net_idx, addr, addr, app_idx, MOD_LF, BT_COMP_ID_LF, NULL);

	// /* Bind to Health model */
	// bt_mesh_cfg_cli_mod_app_bind(net_idx, addr, addr, app_idx, BT_MESH_MODEL_ID_HEALTH_SRV,
	// 			     NULL);

	// /* Add model subscription */
	// bt_mesh_cfg_cli_mod_sub_add_vnd(net_idx, addr, addr, GROUP_ADDR, MOD_LF, BT_COMP_ID_LF,
	// 				NULL);

	printk("Configuration complete\n");

}