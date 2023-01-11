#include <zephyr/kernel.h>
#include <bluetooth/mesh/main.h>
#include <stdbool.h>
#include <stdint.h>

// Initializes buttons and LEDs on the board
int board_init(struct k_work *button_work);

// Turns LED0 on or off
void board_led0_set(bool val);
