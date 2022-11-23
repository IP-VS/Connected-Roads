#include "uart.h"

void uart_init(const struct device* uart_dev) {
    uint32_t dtr = 0;
    if (usb_enable(NULL)) {
        return;
    }
    size_t ms_slept = 0;
    // timeout after 10s
    while (!dtr && ms_slept < 10000) {
        uart_line_ctrl_get(uart_dev, UART_LINE_CTRL_DTR, &dtr);
        /* Give CPU resources to low priority threads. */
        k_sleep(K_MSEC(UART_POLL_TIME_MS));
        ms_slept += UART_POLL_TIME_MS;
    }
    if (!dtr) {
        return;
    }
    if (!device_is_ready(uart_dev)) {
        printk("UART device not found!");
        return;
    }
    /* configure interrupt and callback to receive data */
    uart_irq_rx_enable(uart_dev);
}
void uart_write(const struct device* uart_dev, const char* buf, size_t len) {
    for (int i = 0; i < len; i++) {
        uart_poll_out(uart_dev, buf[i]);
    }
}
bool uart_read_until(const struct device* uart_dev, char* buf, size_t len, char stop) {
    size_t i = 0;
    do {
        if (i > len) {
            return false;
        }
        while (uart_poll_in(uart_dev, &buf[i]) < 0)
            k_sleep(K_MSEC(UART_POLL_TIME_MS));
        ++i;
    } while (buf[i - 1] != stop);
    return true;
}
void uart_read(const struct device* uart_dev, char* buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        while (uart_poll_in(uart_dev, &buf[i]) < 0)
            k_sleep(K_MSEC(UART_POLL_TIME_MS));
    }
}
