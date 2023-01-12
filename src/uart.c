#include "uart.h"
#include "printk.h"

struct k_mutex mtx_read;
struct k_mutex mtx_write;

#define FIFO_BUF_LEN 128
static uint8_t rx_data[FIFO_BUF_LEN];
static int rx_len = 0;

// UART callback function to handle commands
static void uart_fifo_callback(const struct device *dev, void *user_data)
{
	while (uart_fifo_read(dev, &rx_data[rx_len], 1)) {
		if (rx_data[rx_len] == '\n') {
			break;
		}
		rx_len++;
	}
	// Remove last character
	&rx_data[rx_len - 1] == '\0';

	if (rx_len > 1) {
		// Process the received data
		printk("Received %d bytes: %s \n", rx_len, rx_data);

		if (strncmp(rx_data, "SND", 3) == 0) {
			printk("SND command received\n");
			// Send to mesh
			gen_msg_send(&rx_data[3]);
		} else if (strncmp(rx_data, "ADV", 3) == 0) {
			printk("ADV command received\n");
			// Advertise this node as a gateway
			gen_msg_send(&rx_data[0]);
		} else {
			printk("Unknown command received\n");
		}
		// Reset the buffer
		rx_len = 0;
		memset(&rx_data[0], 0, sizeof(rx_data));
	}
}

void uart_init(const struct device *uart_dev)
{
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

void uart_write(const struct device *uart_dev, const char *buf, size_t len)
{
	k_mutex_lock(&mtx_write, K_FOREVER);
	int bytes_written = uart_fifo_fill(uart_dev, buf, len);
	k_mutex_unlock(&mtx_write);
	if (bytes_written <= 0) {
		printf("UART write error or no bytes written\n");
	}
}

bool uart_read_until(const struct device *uart_dev, char *buf, size_t len, char stop)
{
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
void uart_read(const struct device *uart_dev, char *buf, size_t len)
{
	k_mutex_lock(&mtx_read, K_FOREVER);
	int bytes_read = uart_fifo_read(uart_dev, buf, len);
	k_mutex_unlock(&mtx_read);
	if (bytes_read <= 0) {
		printf("UART read error or no bytes available\n");
	}
}
