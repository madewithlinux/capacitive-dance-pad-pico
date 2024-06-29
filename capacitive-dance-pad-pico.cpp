#include <stdio.h>
#include <vector>
#include <array>
#include <numeric>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/timer.h"

#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "touch_sensor_thread.hpp"
#include "multicore_ipc.h"

#include "touch.pio.h"
#include "config.h"

static blink_interval_t blink_interval_ms = BLINK_INIT;

void led_blinking_task(void);
void touch_stats_handler_task();

int main()
{
    board_init();
    tud_init(BOARD_TUD_RHPORT);
    stdio_init_all();
    init_queues();

    multicore_launch_core1(run_touch_sensor_thread);
    while (1)
    {
        touch_stats_handler_task();
        tud_task(); // tinyusb device task
        led_blinking_task();
    }
}

void touch_stats_handler_task()
{
    if (queue_is_empty(&q_touchpad_stats))
    {
        return;
    }

    static absolute_time_t last_timestamp0 = get_absolute_time();
    static absolute_time_t last_timestamp1 = get_absolute_time();

    touchpad_stats_t stats;
    // remove from the queue until it's empty, because we want the most recent one
    while (!queue_is_empty(&q_touchpad_stats))
    {
        queue_try_remove(&q_touchpad_stats, &stats);
    }

    for (int i = 0; i < num_touch_sensors; i++)
    {
        if (stats.by_sensor[i].get_total_count() != stats.by_sensor[0].get_total_count())
        {
            printf("ERROR: sensor %d count mismatch (%d vs %d)\n", i,
                   stats.by_sensor[i].get_total_count(),
                   stats.by_sensor[0].get_total_count());
        }
    }

    uint num_samples = stats.by_sensor[0].get_total_count();

    int64_t reporting_time_us = absolute_time_diff_us(last_timestamp0, last_timestamp1);
    float reporting_freq = ((float)num_samples) / ((float)reporting_time_us / 1.0e6);

    if (log_extra_sensor_info)
    {
        printf("%6.0f %6.0f %6.0f %6.0f||%6.0f %6.0f %6.0f %6.0f | index\n",
               0.0,
               1.0,
               2.0,
               3.0,
               4.0,
               5.0,
               6.0,
               7.0);
        IF_SERIAL_LOG(
            printf("%6u %6u %6u %6u||%6u %6u %6u %6u | threshold\n",
                   touch_sensor_thresholds[0],
                   touch_sensor_thresholds[1],
                   touch_sensor_thresholds[2],
                   touch_sensor_thresholds[3],
                   touch_sensor_thresholds[4],
                   touch_sensor_thresholds[5],
                   touch_sensor_thresholds[6],
                   touch_sensor_thresholds[7]);

            printf("%6.0f %6.0f %6.0f %6.0f||%6.0f %6.0f %6.0f %6.0f | pins\n",
                   (float)touch_sensor_configs[0].pin,
                   (float)touch_sensor_configs[1].pin,
                   (float)touch_sensor_configs[2].pin,
                   (float)touch_sensor_configs[3].pin,
                   (float)touch_sensor_configs[4].pin,
                   (float)touch_sensor_configs[5].pin,
                   (float)touch_sensor_configs[6].pin,
                   (float)touch_sensor_configs[7].pin);
            printf("%6.0f %6.0f %6.0f %6.0f||%6.0f %6.0f %6.0f %6.0f | above threshold\n",
                   stats.by_sensor[0].is_above_threshold() * 111.0,
                   stats.by_sensor[1].is_above_threshold() * 111.0,
                   stats.by_sensor[2].is_above_threshold() * 111.0,
                   stats.by_sensor[3].is_above_threshold() * 111.0,
                   stats.by_sensor[4].is_above_threshold() * 111.0,
                   stats.by_sensor[5].is_above_threshold() * 111.0,
                   stats.by_sensor[6].is_above_threshold() * 111.0,
                   stats.by_sensor[7].is_above_threshold() * 111.0);
            //
        )
    }
    IF_SERIAL_LOG(printf("%6.0f %6.0f %6.0f %6.0f||%6.0f %6.0f %6.0f %6.0f | %d, %5.0f Hz\n",
                         stats.by_sensor[0].get_mean_float(),
                         stats.by_sensor[1].get_mean_float(),
                         stats.by_sensor[2].get_mean_float(),
                         stats.by_sensor[3].get_mean_float(),
                         stats.by_sensor[4].get_mean_float(),
                         stats.by_sensor[5].get_mean_float(),
                         stats.by_sensor[6].get_mean_float(),
                         stats.by_sensor[7].get_mean_float(),
                         reporting_time_us,
                         reporting_freq));
    if (log_extra_sensor_info)
    {
        puts("\n");
    }

    last_timestamp0 = last_timestamp1;
    last_timestamp1 = get_absolute_time();
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
    static uint32_t start_ms = 0;
    static bool led_state = false;

    if (!queue_is_empty(&q_blink_interval))
    {
        queue_try_remove(&q_blink_interval, &blink_interval_ms);
    }
    if (blink_interval_ms == 0)
    {
        led_state = false;
        board_led_write(led_state);
        return;
    }

    // Blink every interval ms
    if (board_millis() - start_ms < blink_interval_ms)
        return; // not enough time
    start_ms += blink_interval_ms;

    board_led_write(led_state);
    led_state = 1 - led_state; // toggle
}
