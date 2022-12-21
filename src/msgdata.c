#include "msgdata.h"

#define OP_ONOFF_GET BT_MESH_MODEL_OP_2(0x82, 0x01)
#define OP_ONOFF_SET BT_MESH_MODEL_OP_2(0x82, 0x02)
#define OP_ONOFF_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x03)
#define OP_ONOFF_STATUS BT_MESH_MODEL_OP_2(0x82, 0x04)

static void attention_on(struct bt_mesh_model* mod) {
    board_led_set(true);
}

static void attention_off(struct bt_mesh_model* mod) {
    board_led_set(false);
}

static const struct bt_mesh_health_srv_cb health_cb = {
    .attn_on = attention_on,
    .attn_off = attention_off,
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

static inline uint8_t model_time_encode(int32_t ms) {
    if (ms == SYS_FOREVER_MS) {
        return 0x3f;
    }

    for (int i = 0; i < ARRAY_SIZE(time_res); i++) {
        if (ms >= BIT_MASK(6) * time_res[i]) {
            continue;
        }

        uint8_t steps = ceiling_fraction(ms, time_res[i]);

        return steps | (i << 6);
    }

    return 0x3f;
}

static int msg_status_send(struct bt_mesh_model* model,
    struct bt_mesh_msg_ctx* ctx) {
    uint32_t remaining;

    BT_MESH_MODEL_BUF_DEFINE(buf, OP_ONOFF_STATUS, 3);
    bt_mesh_model_msg_init(&buf, OP_ONOFF_STATUS);

    remaining = k_ticks_to_ms_floor32(
                    k_work_delayable_remaining_get(&msg.work))
        + msg.transition_time;

    /* Check using remaining time instead of "work pending" to make the
     * msg status send the right value on instant transitions. As the
     * work item is executed in a lower priority than the mesh message
     * handler, the work will be pending even on instant transitions.
     */
    if (remaining) {
        net_buf_simple_add_le16(&buf, msg.len);
        net_buf_simple_add_u8(&buf, model_time_encode(remaining));
    } else {
        net_buf_simple_add_u8(&buf, msg.val);
    }

    return bt_mesh_model_send(model, ctx, &buf, NULL, NULL);
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
        board_led_set(true);

        k_work_reschedule(&msg.work, K_MSEC(msg.transition_time));
        msg.transition_time = 0;
        return;
    }

    board_led_set(!received);
}

/* Generic OnOff Server message handlers */

static int gen_msg_get(struct bt_mesh_model* model,
    struct bt_mesh_msg_ctx* ctx,
    struct net_buf_simple* buf) {
    // msg_status_send(model, ctx);
    return 0;
}

// Receiving the message and handling it
static int gen_msg_set_unack(struct bt_mesh_model* model,
    struct bt_mesh_msg_ctx* ctx,
    struct net_buf_simple* buf) {
    // First 16 bits set the msg length
    uint16_t len = net_buf_simple_pull_le16(buf);
    char msg_str[len + 1]; // Why does this work? o.o
    for (uint16_t i = 0; i < len; i++) {
        char c = net_buf_simple_pull_u8(buf);
        msg_str[i] = c;
    }
    msg_str[len] = '\0';

    if (strlen(msg_str) == 0) {
        // handle the error
        printk("Error: msg_str empty");
        return;
    }

    uint8_t tid = net_buf_simple_pull_u8(buf);
    int32_t trans = 0;
    int32_t delay = 0;

    if (buf->len) {
        trans = model_time_decode(net_buf_simple_pull_u8(buf));
        delay = net_buf_simple_pull_u8(buf) * 5;
    }

    /* Only perform change if the message wasn't a duplicate and the
     * value is different.
     */
    if (tid == msg.tid && ctx->addr == msg.src) {
        /* Duplicate */
        return 0;
    }

    printk("Message received: %s\r\n", msg_str);

    for (int i = 0; i < buf->len; i++) {
        printf("%02x ", buf->data[i]);
    }

    msg.tid = tid;
    msg.src = ctx->addr;
    msg.val = msg_str;
    msg.len = len;
    msg.transition_time = trans;

    /* Schedule the next action to happen on the delay, and keep
     * transition time stored, so it can be applied in the timeout.
     */
    k_work_reschedule(&msg.work, K_MSEC(delay));

    free(msg_str);
    return 0;
}

static int gen_msg_set(struct bt_mesh_model* model,
    struct bt_mesh_msg_ctx* ctx,
    struct net_buf_simple* buf) {
    printk("gen_msg_set\n");
    (void)gen_msg_set_unack(model, ctx, buf);
    // msg_status_send(model, ctx);

    return 0;
}

static const struct bt_mesh_model_op gen_msg_srv_op[] = {
    { OP_ONOFF_GET, BT_MESH_LEN_EXACT(0), gen_msg_get },
    { OP_ONOFF_SET, BT_MESH_LEN_MIN(2), gen_msg_set },
    { OP_ONOFF_SET_UNACK, BT_MESH_LEN_MIN(2), gen_msg_set_unack },
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

        printk("OnOff status: %s -> %s: (%d ms)\n", present,
            target, remaining_time);
        return 0;
    }

    printk("OnOff status: %s\n", present);

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

    board_output_number(action, number);

    return 0;
}

static void prov_complete(uint16_t net_idx, uint16_t addr) {
    board_prov_complete();
}

static void prov_reset(void) {
    bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
}

static uint8_t dev_uuid[16];

static const struct bt_mesh_prov prov = {
    .uuid = dev_uuid,
    .output_size = 4,
    .output_actions = BT_MESH_DISPLAY_NUMBER,
    .output_number = output_number,
    .complete = prov_complete,
    .reset = prov_reset,
};

// Send a message Generic Client to all nodes.
int gen_msg_send(char* msg_str) {
    struct bt_mesh_msg_ctx ctx = {
        .app_idx = models[3].keys[0], /* Use the bound key */
        .addr = BT_MESH_ADDR_ALL_NODES,
        .send_ttl = BT_MESH_TTL_DEFAULT,
    };
    static uint8_t tid;

    if (ctx.app_idx == BT_MESH_KEY_UNUSED) {
        printk("The Generic OnOff Client must be bound to a key before "
               "sending.\n");
        return -ENOENT;
    }

    uint16_t len = strlen(msg_str);

    BT_MESH_MODEL_BUF_DEFINE(buf, OP_ONOFF_SET_UNACK, len * 2); // 2 2x8
    bt_mesh_model_msg_init(&buf, OP_ONOFF_SET_UNACK);

    // First 16 bits of message are the message length
    net_buf_simple_add_le16(&buf, len);
    for (uint16_t i = 0; i < len; i++) {
        net_buf_simple_add_u8(&buf, msg_str[i]);
    }

    printk("Sending message: %s\n", msg_str);

    return bt_mesh_model_send(&models[3], &ctx, &buf, NULL, NULL);
}

static void button_pressed(struct k_work* work) {
    if (bt_mesh_is_provisioned()) {
        (void)gen_msg_send("hello world");
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
}
