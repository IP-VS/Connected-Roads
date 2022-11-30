#include "provision.h"
#include "custom_assert.h"
#include "ip_model.h"

#include <zephyr/drivers/hwinfo.h>

static const uint16_t net_idx;
static const uint16_t app_idx;
static uint16_t self_addr = 1, node_addr;
static uint8_t dev_uuid[16];
static uint8_t node_uuid[16];

void dev_uuid_init() {
    // hwinfo_get_device_id(dev_uuid, 16);
    bt_rand(dev_uuid, 16);
}

K_SEM_DEFINE(sem_unprov_beacon, 0, 1);
K_SEM_DEFINE(sem_node_added, 0, 1);
#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
K_SEM_DEFINE(sem_button_pressed, 0, 1);
#endif

const struct bt_mesh_model_op _opcode_list[] = {
    { MESSAGE_SET_OPCODE, MESSAGE_SET_LEN, handle_message_set },
    { MESSAGE_ACK_OPCODE, MESSAGE_ACK_LEN, handle_message_ack },
    { MESSAGE_STATUS_OPCODE, MESSAGE_STATUS_LEN, handle_message_status },
    BT_MESH_MODEL_OP_END,
};

static struct bt_mesh_cfg_cli cfg_cli = {};

static struct bt_mesh_health_cli health_cli = {
    .current_status = health_current_status,
};

/*
static struct bt_mesh_model_pub pub_ctx = {
    .msg = NET_BUF_SIMPLE(BT_MESH_MODEL_BUF_LEN(MESSAGE_SET_OPCODE, MESSAGE_SET_LEN)),
};
*/

static int msg_model_start(struct bt_mesh_model* model) {
    printk("Started message model with key 0x%04x\r\n", model->keys[0]);
}

static struct bt_mesh_model_cb model_cbs = {
    .start = msg_model_start,
};

static struct bt_mesh_model root_models[]
    = {
          BT_MESH_MODEL_CFG_SRV,
          BT_MESH_MODEL_CFG_CLI(&cfg_cli),
          BT_MESH_MODEL_HEALTH_CLI(&health_cli),
          BT_MESH_MODEL_VND_CB(COMPANY_ID, MODEL_ID, _opcode_list, NULL, NULL, &model_cbs),
      };

static struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(0, root_models, BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
    .cid = BT_COMP_ID_LF,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

static const struct bt_mesh_prov prov = {
    .uuid = dev_uuid,
    .unprovisioned_beacon = unprovisioned_beacon,
    .node_added = node_added,
};

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, { 0 });
static struct gpio_callback button_cb_data;

static const struct bt_mesh_comp node_comp = {
    .cid = CONFIG_BT_COMPANY_ID,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

static const struct bt_mesh_prov node_prov = {
    .uuid = dev_uuid,
};

void health_current_status(struct bt_mesh_health_cli* cli, uint16_t addr,
    uint8_t test_id, uint16_t cid, uint8_t* faults,
    size_t fault_count) {
    size_t i;
    assert_not_null(faults);

    printk("Health Current Status from 0x%04x\r\n", addr);

    if (!fault_count) {
        printk("Health Test ID 0x%02x Company ID 0x%04x: no faults\r\n",
            test_id, cid);
        return;
    }

    printk("Health Test ID 0x%02x Company ID 0x%04x Fault Count %zu:\r\n",
        test_id, cid, fault_count);

    for (i = 0; i < fault_count; i++) {
        printk("\t0x%02x\r\n", faults[i]);
    }
}

void setup_cdb(void) {
    struct bt_mesh_cdb_app_key* key;

    key = bt_mesh_cdb_app_key_alloc(net_idx, app_idx);
    if (key == NULL) {
        printk("Failed to allocate app-key 0x%04x\r\n", app_idx);
        return;
    }

    bt_rand(key->keys[0].app_key, 16);

    if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
        bt_mesh_cdb_app_key_store(key);
    }
}

void configure_self(struct bt_mesh_cdb_node* self) {
    assert_not_null(self);
    struct bt_mesh_cdb_app_key* key;
    uint8_t status = 0;
    int err;

    printk("Configuring self...\r\n");

    key = bt_mesh_cdb_app_key_get(app_idx);
    if (key == NULL) {
        printk("No app-key 0x%04x\r\n", app_idx);
        return;
    }

    /* Add Application Key */
    err = bt_mesh_cfg_app_key_add(self->net_idx, self->addr, self->net_idx,
        app_idx, key->keys[0].app_key, &status);
    if (err || status) {
        printk("Failed to add app-key (err %d, status %d)\r\n", err,
            status);
        return;
    }

    err = bt_mesh_cfg_mod_app_bind(self->net_idx, self->addr, self->addr,
        app_idx, BT_MESH_MODEL_ID_HEALTH_CLI,
        &status);
    if (err || status) {
        printk("Failed to bind app-key (err %d, status %d)\r\n", err,
            status);
        return;
    }

    atomic_set_bit(self->flags, BT_MESH_CDB_NODE_CONFIGURED);

    if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
        bt_mesh_cdb_node_store(self);
    }

    printk("Configuration complete\r\n");
}
void configure_node(struct bt_mesh_cdb_node* node) {
    NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_RX_SDU_MAX);
    struct bt_mesh_comp_p0_elem elem;
    struct bt_mesh_cdb_app_key* key;
    struct bt_mesh_comp_p0 comp;
    uint8_t status;
    int err, elem_addr;

    assert_not_null(node);

    printk("Configuring node 0x%04x...\r\n", node->addr);

    key = bt_mesh_cdb_app_key_get(app_idx);
    if (key == NULL) {
        printk("No app-key 0x%04x\r\n", app_idx);
        return;
    }

    /* Add Application Key */
    err = bt_mesh_cfg_app_key_add(net_idx, node->addr, net_idx, app_idx,
        key->keys[0].app_key, &status);
    if (err || status) {
        printk("Failed to add app-key (err %d status %d)\r\n", err, status);
        return;
    }

    /* Get the node's composition data and bind all models to the appkey */
    err = bt_mesh_cfg_comp_data_get(net_idx, node->addr, 0, &status, &buf);
    if (err || status) {
        printk("Failed to get Composition data (err %d, status: %d)\r\n",
            err, status);
        return;
    }

    err = bt_mesh_comp_p0_get(&comp, &buf);
    if (err) {
        printk("Unable to parse composition data (err: %d)\r\n", err);
        return;
    }

    elem_addr = node->addr;
    while (bt_mesh_comp_p0_elem_pull(&comp, &elem)) {
        printk("Element @ 0x%04x: %u + %u models\r\n", elem_addr,
            elem.nsig, elem.nvnd);
        for (int i = 0; i < elem.nsig; i++) {
            uint16_t id = bt_mesh_comp_p0_elem_mod(&elem, i);

            if (id == BT_MESH_MODEL_ID_CFG_CLI || id == BT_MESH_MODEL_ID_CFG_SRV) {
                continue;
            }
            printk("Binding AppKey to model 0x%03x:%04x\r\n",
                elem_addr, id);

            err = bt_mesh_cfg_mod_app_bind(net_idx, node->addr,
                elem_addr, app_idx, id,
                &status);
            if (err || status) {
                printk("Failed (err: %d, status: %d)\r\n", err,
                    status);
            }
        }

        for (int i = 0; i < elem.nvnd; i++) {
            struct bt_mesh_mod_id_vnd id = bt_mesh_comp_p0_elem_mod_vnd(&elem, i);

            printk("Binding AppKey to model 0x%03x:%04x:%04x\r\n",
                elem_addr, id.company, id.id);

            err = bt_mesh_cfg_mod_app_bind_vnd(net_idx, node->addr,
                elem_addr, app_idx,
                id.id, id.company,
                &status);
            if (err || status) {
                printk("Failed (err: %d, status: %d)\r\n", err,
                    status);
            }
        }

        elem_addr++;
    }

    atomic_set_bit(node->flags, BT_MESH_CDB_NODE_CONFIGURED);

    if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
        bt_mesh_cdb_node_store(node);
    }

    printk("Configuration complete\r\n");
}
void unprovisioned_beacon(uint8_t uuid[16],
    bt_mesh_prov_oob_info_t oob_info,
    uint32_t* uri_hash) {
    assert_not_null(uuid);
    memcpy(node_uuid, uuid, 16);
    assert_equal(k_sem_count_get(&sem_unprov_beacon), 0);
    k_sem_give(&sem_unprov_beacon);
}
void node_added(uint16_t net_idx, uint8_t uuid[16], uint16_t addr, uint8_t num_elem) {
    printk("Node added: net_idx=%04x, addr=%04x, num_elem=%d\r\n", net_idx, addr, num_elem);
    node_addr = addr;
    assert_equal(k_sem_count_get(&sem_node_added), 0);
    k_sem_give(&sem_node_added);
    printk("Sem given for node_added\r\n");
}
int bt_ready(void) {
    uint8_t net_key[16], dev_key[16];
    int err;

    err = bt_mesh_init(&prov, &comp);
    if (err) {
        printk("Initializing mesh failed (err %d)\r\n", err);
        return err;
    }

    printk("Mesh initialized\r\n");

    if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
        printk("Loading stored settings\r\n");
        settings_load();
    }

    bt_rand(net_key, 16);

    err = bt_mesh_cdb_create(net_key);
    if (err == -EALREADY) {
        printk("Using stored CDB\r\n");
    } else if (err) {
        printk("Failed to create CDB (err %d)\r\n", err);
        return err;
    } else {
        printk("Created CDB\r\n");
        setup_cdb();
    }

    bt_rand(dev_key, 16);

    err = bt_mesh_provision(net_key, BT_MESH_NET_PRIMARY, 0, 0, self_addr,
        dev_key);
    if (err == -EALREADY) {
        printk("Using stored settings\r\n");
    } else if (err) {
        printk("Provisioning failed (err %d)\r\n", err);
        return err;
    } else {
        printk("Provisioning completed\r\n");
    }

    return 0;
}
uint8_t check_unconfigured(struct bt_mesh_cdb_node* node, void* data) {
    assert_not_null(node);
    if (!atomic_test_bit(node->flags, BT_MESH_CDB_NODE_CONFIGURED)) {
        printk("Node %p's flags say it's not configured\r\n", (const void*)node);
        if (node->addr == self_addr) {
            printk("Self (%p) not configured, configuring\r\n", (const void*)node);
            configure_self(node);
        } else {
            printk("Node (%p) not configured, configuring\r\n", (const void*)node);
            configure_node(node);
        }
    }

    return BT_MESH_CDB_ITER_CONTINUE;
}
void button_pressed(const struct device* dev, struct gpio_callback* cb, uint32_t pins) {
    assert_equal(k_sem_count_get(&sem_button_pressed), 0);
    k_sem_give(&sem_button_pressed);
}
void button_init(void) {
    int ret;

    assert_not_null(button.port);

    if (!device_is_ready(button.port)) {
        printk("Error: button device %s is not ready\r\n", button.port->name);
        return;
    }
    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) {
        printk("Error %d: failed to configure %s pin %d\r\n", ret, button.port->name,
            button.pin);
        return;
    }
    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        printk("Error %d: failed to configure interrupt on %s pin %d\r\n", ret,
            button.port->name, button.pin);
        return;
    }
    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);
    printk("Buttons are ready!\r\n");
}
bool wait_for_button_press(int timeout_s) {
    k_sem_reset(&sem_button_pressed);
    int err = k_sem_take(&sem_button_pressed, K_SECONDS(timeout_s));
    if (err == -EAGAIN) {
        printk("Timed out, button 1 wasn't pressed in time.\r\n");
        return false;
    }
    printk("Button 1 was pressed!\r\n");
    return true;
}
void provision(void) {
    char uuid_hex_str[32 + 1];
    int err;

    printk("Initializing...\r\n");

    /* Initialize the Bluetooth Subsystem */
    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\r\n", err);
        return;
    }

    printk("Bluetooth initialized\r\n");
    bt_ready();

#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
    button_init();
#endif

    while (1) {
        printk("Resetting unprov beacon semaphore\r\n");
        k_sem_reset(&sem_unprov_beacon);
        printk("Resetting node added semaphore\r\n");
        k_sem_reset(&sem_node_added);
        printk("Checking for unconfigured nodes\r\n");
        bt_mesh_cdb_node_foreach(check_unconfigured, NULL);

        printk("Waiting for unprovisioned beacon...\r\n");
        err = k_sem_take(&sem_unprov_beacon, K_SECONDS(10));
        if (err == -EAGAIN) {
            continue;
        }

        bin2hex(node_uuid, 16, uuid_hex_str, sizeof(uuid_hex_str));

        /*
        #if DT_NODE_HAS_STATUS(SW0_NODE, okay)
                k_sem_reset(&sem_button_pressed);
                printk("Device %s detected, press button 1 to provision.\r\n", uuid_hex_str);
                err = k_sem_take(&sem_button_pressed, K_SECONDS(30));
                if (err == -EAGAIN) {
                    printk("Timed out, button 1 wasn't pressed in time.\r\n");
                    continue;
                } else if (err == -EBUSY) {
                    printk("Button pressed semaphore taken without waiting\r\n");
                }
        #endif
        */

        printk("Provisioning %s\r\n", uuid_hex_str);
        err = bt_mesh_provision_adv(node_uuid, net_idx, 0, 0);
        if (err < 0) {
            printk("Provisioning failed (err %d)\r\n", err);
            continue;
        }

        printk("Waiting for node to be added...\r\n");
        err = k_sem_take(&sem_node_added, K_SECONDS(10));
        if (err == -EAGAIN) {
            printk("Timeout waiting for node to be added\r\n");
            continue;
        } else if (err == -EBUSY) {
            printk("Node adding semaphore taken without waiting\r\n");
        }

        printk("Added node 0x%04x\r\n", node_addr);
    }
}
int run_bt_node(void) {
    uint8_t net_key[16], dev_key[16];
    int err;

    err = bt_mesh_init(&node_prov, &node_comp);
    if (err) {
        printk("Initializing mesh failed (err %d)\r\n", err);
        return 1;
    }

    printk("Mesh initialized\r\n");

    if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
        printk("Loading stored settings\r\n");
        settings_load();
    }

    err = bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);
    if (err) {
        printk("bt_mesh_prov_enable failed (err %d)\r\n", err);
    } else {
        printk("bt_mesh_prov_enable ok\r\n");
    }

    while (1) {
        for (uint16_t i = 1; i < 4; ++i) {
            int err = send_message(&root_models[3], i);
            if (err) {
                printk("Error sending to 0x%04x: %d\r\n", i, err);
            } else {
                printk("Successfully sent message to 0x%04x\r\n", i);
            }
        }
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
