#include "multicore_ipc.h"
#include "touch_sensor_thread.hpp"

queue_t q_blink_interval;
queue_t q_touchpad_stats;

void init_queues() {
  queue_init(&q_blink_interval, sizeof(blink_interval_t), 10);
  queue_init(&q_touchpad_stats, sizeof(touchpad_stats_t), 1);
}
