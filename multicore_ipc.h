#pragma once

#include "pico/util/queue.h"

enum blink_interval_t
{
    BLINK_INIT = 1000,

    BLINK_SENSORS_INIT = 100,
    BLINK_SENSORS_CALIBRATING = 250,
    BLINK_SENSORS_OK = 0,

    BLINK_NOT_MOUNTED = 1500,
    BLINK_MOUNTED = 1000,
    BLINK_SUSPENDED = 2500,

    BLINK_ALWAYS_ON = UINT32_MAX,
    BLINK_ALWAYS_OFF = 0
};

// queues for core1 to send data to core0
extern queue_t q_blink_interval;
extern queue_t q_touchpad_stats;
// queues for core0 to send data to core1
// TODO: none so far


void init_queues();
