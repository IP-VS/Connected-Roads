#include "uart.h"

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart),
    "Console device is not ACM CDC UART device");

static const struct device* uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

struct k_mutex uart_mtx;

// Whether UART was initialized properly - this is not a
// fatal failure, so we store it and ignore any uart_* calls.
static bool s_uart_is_initialized = false;

void uart_init(void) {
    k_mutex_init(&uart_mtx);
    k_mutex_lock(&uart_mtx, K_FOREVER);
    uint32_t dtr = 0;
    if (usb_enable(NULL)) {
        return;
    }

    size_t ms_slept = 0;
    // timeout after 5s
    while (!dtr && ms_slept < 5000) {
        uart_line_ctrl_get(uart_dev, UART_LINE_CTRL_DTR, &dtr);
        /* Give CPU resources to low priority threads. */
        k_sleep(K_MSEC(UART_POLL_TIME_MS));
        ms_slept += UART_POLL_TIME_MS;
    }
    if (!dtr) {
        goto end;
    }
    if (!device_is_ready(uart_dev)) {
        printk("UART device not found!");
        goto end;
    }
    /* configure interrupt and callback to receive data */
    uart_irq_rx_enable(uart_dev);
    s_uart_is_initialized = true;
end:
    k_mutex_unlock(&uart_mtx);
}

void uart_write(const char* buf, size_t len) {
    k_mutex_lock(&uart_mtx, K_FOREVER);
    if (!s_uart_is_initialized) {
        goto end;
    }
    for (int i = 0; i < len; i++) {
        uart_poll_out(uart_dev, buf[i]);
    }
end:
    k_mutex_unlock(&uart_mtx);
}

bool uart_read_until(char* buf, size_t len, char stop) {
    k_mutex_lock(&uart_mtx, K_FOREVER);
    bool result = true;
    if (!s_uart_is_initialized) {
        result = false;
        goto end;
    }

    size_t i = 0;
    do {
        if (i > len) {
            result = false;
            goto end;
        }
        while (uart_poll_in(uart_dev, &buf[i]) < 0)
            k_sleep(K_MSEC(UART_POLL_TIME_MS));
        ++i;
    } while (buf[i - 1] != stop);

end:
    k_mutex_unlock(&uart_mtx);
    return result;
}
void uart_read(char* buf, size_t len) {
    k_mutex_lock(&uart_mtx, K_FOREVER);
    if (!s_uart_is_initialized) {
        goto end;
    }
    for (size_t i = 0; i < len; ++i) {
        while (uart_poll_in(uart_dev, &buf[i]) < 0)
            k_sleep(K_MSEC(UART_POLL_TIME_MS));
    }
end:
    k_mutex_unlock(&uart_mtx);
}
