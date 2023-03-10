#include "uart.h"
#include "printk.h"

#include <ctype.h>
#include <msgdata.h>
#include <stdlib.h>

struct k_mutex mtx_read;
struct k_mutex mtx_write;

#define FIFO_BUF_LEN 128
static uint8_t rx_data[FIFO_BUF_LEN];
static uint8_t clean_data[FIFO_BUF_LEN];
static int rx_len = 0;

// UART callback function to handle commands
static void uart_fifo_callback(const struct device* dev, void* user_data) {
    while (uart_fifo_read(dev, &rx_data[rx_len], 1)) {
        // Copy non-alphanumeric characters from rx_data into clean_data
        if (isalnum(rx_data[rx_len])) {
            clean_data[rx_len] = rx_data[rx_len];
            rx_len++;
        }
    }
    clean_data[rx_len] = '\0';

    if (rx_len > 1) {
        // Process the received data
        printk("Received %d bytes: %s \n", rx_len, clean_data);

        if (strncmp(clean_data, "SND", 3) == 0) {
            printk("SND command received\n");
            // Send to mesh
            gen_msg_send(MSG_SND_COMM, &clean_data[3], (size_t)rx_len - 3 + 1);
        } else if (strncmp(clean_data, "ADV", 3) == 0) {
            printk("ADV command received\n");
            // Advertise this node as a gateway
            int len = sprintf(&clean_data[3], "%d", primary_addr);
            gen_msg_send(MSG_ADV_COMM, &clean_data[3], (size_t)len + 1);
        } else if (strncmp(clean_data, "UPT", 3) == 0) {
            printk("UPT command received\n");
            // Send uptime command
            gen_msg_send(MSG_UPTIME, &clean_data[0], (size_t)rx_len + 1);
        } else if (strncmp(clean_data, "REM", 3) == 0) {
            // REMabcdef
            // removes node with hwid 'abcdef'
            printk("REM command received\n");
            if (rx_len > 3) {
                char* end;
                unsigned long addr = strtoul(&clean_data[3], &end, 16);
                if (addr == ULONG_MAX) {
                    printk("REM command address failed to parse as a hexadecimal number: %s\n", strerror(errno));
                } else {
                    printk("REM command parsed with address '%lx', sending...\n", addr);
                    gen_msg_send(MSG_REMOVE, &addr, sizeof(addr));
                }
            } else {
                printk("REM command provided with no address argument, ignoring!\n");
            }
        } else {
            printk("Unknown command received\n");
        }
        // Reset the buffer
        rx_len = 0;
        memset(&clean_data[0], 0, sizeof(clean_data));
    }
}

void uart_init(const struct device* uart_dev) {
    uint32_t dtr = 0;
    if (usb_enable(NULL)) {
        return;
    }
    /* configure interrupt and callback to receive data */
    uart_irq_callback_set(uart_dev, uart_fifo_callback);
    uart_irq_rx_enable(uart_dev);
    printk("Set UART FIFO cb. Ready to receive data...");

    k_mutex_init(&mtx_read);
    k_mutex_init(&mtx_write);
    size_t ms_slept = 0;
    // timeout after 10s
    while (!dtr && ms_slept < 2000) {
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
}

void uart_write(const struct device* uart_dev, const char* buf, size_t len) {
    k_mutex_lock(&mtx_write, K_FOREVER);
    int bytes_written = uart_fifo_fill(uart_dev, buf, len);
    k_mutex_unlock(&mtx_write);
    if (bytes_written <= 0) {
        printf("UART write error or no bytes written\n");
    }
}

bool uart_read_until(const struct device* uart_dev, char* buf, size_t len, char stop) {
    k_mutex_lock(&mtx_read, K_FOREVER);
    int i = 0;
    int bytes_read;
    do {
        if (i > len) {
            k_mutex_unlock(&mtx_read);
            return false;
        }
        bytes_read = uart_fifo_read(uart_dev, &buf[i], 1);
        if (bytes_read <= 0) {
            k_mutex_unlock(&mtx_read);
            return false;
        }
        ++i;
    } while (buf[i - 1] != stop);
    k_mutex_unlock(&mtx_read);
    return true;
}
void uart_read(const struct device* uart_dev, char* buf, size_t len) {
    k_mutex_lock(&mtx_read, K_FOREVER);
    int bytes_read = uart_fifo_read(uart_dev, buf, len);
    k_mutex_unlock(&mtx_read);
    if (bytes_read <= 0) {
        printf("UART read error or no bytes available\n");
    }
}
