#include "msgdata.h"
#include "datastructures.h"
#include <stdint.h>
#include <zephyr/net/buf.h>

#define OP_ONOFF_GET BT_MESH_MODEL_OP_2(0x82, 0x01)
#define OP_ONOFF_SET BT_MESH_MODEL_OP_2(0x82, 0x02)
#define OP_ONOFF_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x03)
#define OP_ONOFF_STATUS BT_MESH_MODEL_OP_2(0x82, 0x04)

static struct device* uart_dev;
static char* uart_buffer[5]; // Create buffer for uart read
uint16_t primary_addr = 0; // This node's address
unsigned int recv_addr = BT_MESH_ADDR_ALL_NODES;
static struct k_sem prov_sem;
static struct bt_mesh_model models[];
static bool configured = false;

// Node added cb (provisioning complete and configured)
static void prov_node_added(uint16_t net_idx, uint8_t uuid[16], uint16_t addr, uint8_t num_elem) {
    printk("Device 0x%04x provisioned", addr);
    k_sem_give(&prov_sem);
}
#define OP_GENERIC_MSG BT_MESH_MODEL_OP_2(0x82, 0x03)
#define OP_ONOFF_STATUS BT_MESH_MODEL_OP_2(0x82, 0x04)

static void health_srv_attention_on(struct bt_mesh_model* mod) {
    board_led0_set(true);
}

static void health_srv_attention_off(struct bt_mesh_model* mod) {
    board_led0_set(false);
}

static const struct bt_mesh_health_srv_cb health_cb = {
    .attn_on = health_srv_attention_on,
    .attn_off = health_srv_attention_off,
};

static struct bt_mesh_health_srv health_srv = {
    .cb = &health_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static struct {
    char* val;
    uint16_t len;
    uint8_t tid;
    uint16_t src;
    uint32_t transition_time;
    struct k_work_delayable work;
} msg;

/* OnOff messages' transition time and remaining time fields are encoded as an
 * 8 bit value with a 6 bit step field and a 2 bit resolution field.
 * The resolution field maps to:
 * 0: 100 ms
 * 1: 1 s
 * 2: 10 s
 * 3: 20 min
 */
static const uint32_t time_res[] = {
    100,
    MSEC_PER_SEC,
    10 * MSEC_PER_SEC,
    10 * 60 * MSEC_PER_SEC,
};

static inline int32_t model_time_decode(uint8_t val) {
    uint8_t resolution = (val >> 6) & BIT_MASK(2);
    uint8_t steps = val & BIT_MASK(6);

    if (steps == 0x3f) {
        return SYS_FOREVER_MS;
    }

    return steps * time_res[resolution];
}

static bool received = false;
static void msg_timeout(struct k_work* work) {
    if (msg.transition_time) {
        /* Start transition.
         *
         * The LED should be on as long as the transition is in
         * progress, regardless of the target value, according to the
         * Bluetooth Mesh Model specification, section 3.1.1.
         */
        board_led0_set(true);

        k_work_reschedule(&msg.work, K_MSEC(msg.transition_time));
        msg.transition_time = 0;
        return;
    }

    board_led0_set(!received);
}

/* Generic OnOff Server message handlers */

static int gen_msg_get(struct bt_mesh_model* model,
    struct bt_mesh_msg_ctx* ctx,
    struct net_buf_simple* buf) {
    // msg_status_send(model, ctx);
    return 0;
}

// Receiving the message and handling it
static int gen_msg_generic(struct bt_mesh_model* model,
    struct bt_mesh_msg_ctx* ctx,
    struct net_buf_simple* buf) {

    enum msg_type type = net_buf_simple_pull_u8(buf);
    uint16_t len = net_buf_simple_pull_le16(buf);
    uint8_t* msg_buf = net_buf_simple_pull_mem(buf, len);

    switch (type) {
    case MSG_HELLO:
        printk("bt: got hello message: '%s'\n", msg_buf);
        break;
    case MSG_HEARTBEAT:
        printk("bt: got heartbeat message: '%s'\n", msg_buf);
        break;
    case MSG_SND_COMM:
        printk("bt: got SND message: '%s'\n", msg_buf);
        break;
    case MSG_MIC_DATA:
        printk("bt: got MIC message: '%s'\n", msg_buf);
        // Write the data to the uart
        printk("micdata: %s\n", msg_buf);
        break;
    case MSG_ADV_COMM:
        printk("bt: got ADV message: '%s'\n", msg_buf);
        int adv_addr = atoi(msg_buf);
        printk("ADV command received %d %s\n", adv_addr, msg_buf);
        // Compare msg_buf with the address of the device
        if (primary_addr != adv_addr) {
            printk("Setup gateway ID: %s\n", msg_buf);
            recv_addr = (unsigned int)adv_addr;
        }
        break;
    default:
        printk("Error: Unhandled bt message type '%c' (%d)\n", type, type);
        return -1;
    }

    return 0;
}

static int gen_msg_set(struct bt_mesh_model* model,
    struct bt_mesh_msg_ctx* ctx,
    struct net_buf_simple* buf) {
    printk("gen_msg_set\n");
    (void)gen_msg_generic(model, ctx, buf);
    // msg_status_send(model, ctx);

    return 0;
}

static const struct bt_mesh_model_op gen_msg_srv_op[] = {
    { OP_ONOFF_GET, BT_MESH_LEN_EXACT(0), gen_msg_get },
    { OP_ONOFF_SET, BT_MESH_LEN_MIN(2), gen_msg_set },
    { OP_GENERIC_MSG, BT_MESH_LEN_MIN(2), gen_msg_generic },
    BT_MESH_MODEL_OP_END,
};

/* Generic OnOff Client */

static int gen_msg_status(struct bt_mesh_model* model,
    struct bt_mesh_msg_ctx* ctx,
    struct net_buf_simple* buf) {
    uint8_t present = net_buf_simple_pull_u8(buf);

    if (buf->len) {
        uint8_t target = net_buf_simple_pull_u8(buf);
        int32_t remaining_time = model_time_decode(net_buf_simple_pull_u8(buf));

        printk("OnOff status: %c -> %c: (%d ms)\n", present,
            target, remaining_time);
        return 0;
    }

    printk("OnOff status: %c\n", present);

    return 0;
}

static const struct bt_mesh_model_op gen_msg_cli_op[] = {
    { OP_ONOFF_STATUS, BT_MESH_LEN_MIN(1), gen_msg_status },
    BT_MESH_MODEL_OP_END,
};

/* This application only needs one element to contain its models */
static struct bt_mesh_model models[] = {
    BT_MESH_MODEL_CFG_SRV,
    BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_msg_srv_op, NULL,
        NULL),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_CLI, gen_msg_cli_op, NULL,
        NULL),
};

static struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(0, models, BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
    .cid = BT_COMP_ID_LF,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

/* Provisioning */
static int output_number(bt_mesh_output_action_t action, uint32_t number) {
    printk("OOB Number: %u\n", number);

    k_sem_give(&prov_sem);

    return 0;
}

// Provisioning is complete but NOT configured yet
static void prov_complete(uint16_t net_idx, uint16_t addr) {
    // This function will be called when a BT mesh is provisioned
    // Do something here, such as printing a message or setting a flag
    printk("Provisioning complete: net_idx = %u, addr = %u\n", net_idx, addr);

    primary_addr = addr;
    k_sem_give(&prov_sem);

    // send_msg_from_uart(dev);

    // // Send a message to all nodes
    // gen_msg_send("Hello from the primary node!");
}

static void prov_reset(void) {
    bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
    k_sem_give(&prov_sem);
}

static uint8_t dev_uuid[16];

static const struct bt_mesh_prov prov = {
    .uuid = dev_uuid,
    .output_size = 4,
    .output_actions = BT_MESH_DISPLAY_NUMBER,
    .output_number = output_number,
    .complete = prov_complete,
    .reset = prov_reset,
    .node_added = prov_node_added,
};

// Send a message Generic Client to all nodes.
int gen_msg_send(enum msg_type type, const void* msg_buf, size_t len) {
    printk("gen_msg_send to addr: %d\n", recv_addr);
    struct bt_mesh_msg_ctx ctx = {
        .app_idx = models[3].keys[0], /* Use the bound key */
        .addr = recv_addr,
        .send_ttl = BT_MESH_TTL_DEFAULT,
    };
    static uint8_t tid;

    if (ctx.app_idx == BT_MESH_KEY_UNUSED) {
        printk("The Generic OnOff Client must be bound to a key before "
               "sending.\n");
        return -ENOENT;
    }

    const size_t max = UINT16_MAX - sizeof(uint16_t) - sizeof(uint8_t);
    if (len > max) {
        printk("Error: Tried to send message of length %d, but maximum possible is %d", len, max);
        return -1;
    }

    // size is:
    // 1 byte header
    // 2 bytes length
    // <length> bytes data
    BT_MESH_MODEL_BUF_DEFINE(buf, OP_GENERIC_MSG, (uint16_t)(sizeof(uint8_t) + sizeof(uint16_t) + len));
    bt_mesh_model_msg_init(&buf, OP_GENERIC_MSG);

    net_buf_simple_add_u8(&buf, (uint8_t)type);
    net_buf_simple_add_le16(&buf, (uint16_t)len);
    net_buf_simple_add_mem(&buf, (const void*)msg_buf, len);

    printk("Sending message of length %d\n", len);

    int err = bt_mesh_model_send(&models[3], &ctx, &buf, NULL, NULL);
    if (err) {
        printk("Error: bt_mesh_model_send failed: err=%d\n", err);
    }
    return err;
}

int mic_msg_send(const struct Samples* samples) {
    uint8_t buf[SAMPLES_SERIALIZE_BUFFER_SIZE];
    if (!samples_serialize(buf, sizeof(buf), samples)) {
        printk("Failed to serialize samples!\n");
        return -1;
    }
    return gen_msg_send(MSG_MIC_DATA, buf, sizeof(buf));
}

static void button_pressed(struct k_work* work) {
    if (bt_mesh_is_provisioned()) {
        const char hello_msg[] = "hello world";
        (void)gen_msg_send(MSG_HELLO, hello_msg, sizeof(hello_msg));
        return;
    }

    /* Self-provision with an arbitrary address.
     *
     * NOTE: This should never be done in a production environment.
     *       Addresses should be assigned by a provisioner, and keys should
     *       be generated from true random numbers. It is done in this
     *       sample to allow testing without a provisioner.
     */
    static uint8_t net_key[16];
    static uint8_t dev_key[16];
    static uint8_t app_key[16];
    uint16_t addr;
    int err;

    if (IS_ENABLED(CONFIG_HWINFO)) {
        addr = sys_get_le16(&dev_uuid[0]) & BIT_MASK(15);
    } else {
        addr = k_uptime_get_32() & BIT_MASK(15);
    }

    printk("Self-provisioning with address 0x%04x\n", addr);
    err = bt_mesh_provision(net_key, 0, 0, 0, addr, dev_key);
    if (err) {
        printk("Provisioning failed (err: %d)\n", err);
        return;
    }

    /* Add an application key to both Generic OnOff models: */
    err = bt_mesh_app_key_add(0, 0, app_key);
    if (err) {
        printk("App key add failed (err: %d)\n", err);
        return;
    }

    /* Models must be bound to an app key to send and receive messages with
     * it:
     */
    models[2].keys[0] = 0;
    models[3].keys[0] = 0;

    printk("Provisioned and configured!\n");
}

static void bt_ready(int err) {
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    printk("Bluetooth initialized\n");

    k_sem_init(&prov_sem, 0, 1);
    err = bt_mesh_init(&prov, &comp);
    if (err) {
        printk("Initializing mesh failed (err %d)\n", err);
        return;
    }

    if (IS_ENABLED(CONFIG_SETTINGS)) {
        settings_load();
    }

    /* This will be a no-op if settings_load() loaded provisioning info */
    bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);

    printk("Mesh initialized\n");
}

void msgdata_init(void) {
    uart_dev = dev;
    static struct k_work button_work;
    int err = -1;

    printk("Msg data initializing...\n");

    if (IS_ENABLED(CONFIG_HWINFO)) {
        err = hwinfo_get_device_id(dev_uuid, sizeof(dev_uuid));
    }

    if (err < 0) {
        dev_uuid[0] = 0xdd;
        dev_uuid[1] = 0xdd;
    }

    k_work_init(&button_work, button_pressed);

    err = board_init(&button_work);
    if (err) {
        printk("Board init failed (err: %d)\n", err);
        return;
    }

    k_work_init_delayable(&msg.work, msg_timeout);

    /* Initialize the Bluetooth Subsystem */
    err = bt_enable(bt_ready);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
    }

    // Initialize work queue
    // k_work_init(&send_msg_from_uart_work, send_msg_from_uart);

    // // Work queue with priority 5
    // k_work_queue_start(&send_msg_from_uart_work, work_q_stack, sizeof(work_q_stack),
    // 		   K_PRIO_COOP(5), NULL);
    // // Add work to work queue
    // k_work_submit_to_queue(&send_msg_from_uart_work, send_msg_from_uart);

    // struct k_work_q my_work_q;

    // k_work_queue_init(&my_work_q);

    // k_work_queue_start(&my_work_q, my_stack_area, K_THREAD_STACK_SIZEOF(my_stack_area),
    // 		   MY_PRIORITY, NULL);

    // struct k_work work;
    // k_work_init(&work, send_msg_from_uart);
    // k_work_submit(&work);

    // k_timer_init(&timer, timer_callback, NULL);
    // k_timer_start(&timer, K_SECONDS(5), K_SECONDS(5));
}

void test_init(uint8_t sleeptime) {
    int err;
    if (IS_ENABLED(CONFIG_HWINFO)) {
        err = hwinfo_get_device_id(dev_uuid, sizeof(dev_uuid));
    }

    if (err < 0) {
        dev_uuid[0] = 0xdd;
        dev_uuid[1] = 0xdd;
    }
}
