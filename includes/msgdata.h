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
#include "datastructures.h"

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
    MSG_UPTIME = 'u',
    MSG_UPTIME_ACK = 'k',
};

// Broadcasts a message to the mesh.
int gen_msg_send(enum msg_type type, const void* val, size_t len);

// Broadcasts the provided microphone data as a MSG_MIC_DATA message.
// Convenience function :^)
int mic_msg_send(const struct Samples* samples);

// Initializes the board, gets hardware info, etc. in preparation
// to sending and receiving data.
void msgdata_init(void);
