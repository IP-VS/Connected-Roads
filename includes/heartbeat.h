#ifndef HEARTBEAT_H
#define HEARTBEAT_H

// Heartbeats but not the Zephyr ones just broadcasts 
// an alive message every x seconds

#include <stdlib.h>
#include <zephyr/zephyr.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

void heartbeat_init(uint8_t sleeptime);

#endif