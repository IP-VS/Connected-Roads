#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>

// Initializes buttons and LEDs on the board
int board_init(struct k_work* button_work);

// Turns LED0 on or off
void board_led0_set(bool val);
