#pragma once

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/zephyr.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>

#include "board.h"
#include "printk.h"

/* Receiver */
extern unsigned int recv_addr;
/* My addr */
extern uint16_t primary_addr;

// Human-readable header byte for each message.
enum msg_type {
    MSG_HELLO = 'o',
    MSG_HEARTBEAT = 'h',
    MSG_SND_COMM = 's',
    MSG_ADV_COMM = 'a',
    MSG_MIC_DATA = 'm',
};

// Broadcasts a message to the mesh.
int gen_msg_send(enum msg_type type, const void* val, size_t len);

// Initializes the board, gets hardware info, etc. in preparation
// to sending and receiving data.
void msgdata_init(void);
