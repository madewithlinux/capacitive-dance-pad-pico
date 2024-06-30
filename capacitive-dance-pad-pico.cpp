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
#include "touch_sensor_config.hpp"
#include "touch_sensor_thread.hpp"
#include "multicore_ipc.h"

#include "touch.pio.h"
#include "config.h"

static blink_interval_t blink_interval_ms = BLINK_INIT;

// TODO: some way to make these dependent on which player it is
static uint8_t game_button_to_keycode_map[NUM_GAME_BUTTONS] = {
    [UP] = HID_KEY_ARROW_UP,
    [DOWN] = HID_KEY_ARROW_DOWN,
    [LEFT] = HID_KEY_ARROW_LEFT,
    [RIGHT] = HID_KEY_ARROW_RIGHT,
};

static uint8_t hid_report_keycodes[6] = {0};
static bool hid_report_dirty = true;

void led_blinking_task(void);
void touch_stats_handler_task();
void hid_task();

int main()
{
    board_init();
    tud_init(BOARD_TUD_RHPORT);
    stdio_init_all();
    init_queues();

    multicore_launch_core1(run_touch_sensor_thread);
    while (1)
    {
        tud_task(); // tinyusb device task
        touch_stats_handler_task();
        hid_task();
        led_blinking_task();
    }
}

void log_stats(touchpad_stats_t &stats)
{

    static absolute_time_t last_timestamp0 = get_absolute_time();
    static absolute_time_t last_timestamp1 = get_absolute_time();

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

void touch_stats_handler_task()
{
    if (queue_is_empty(&q_touchpad_stats))
    {
        return;
    }

    touchpad_stats_t stats;
    // remove from the queue until it's empty, because we want the most recent one
    while (!queue_is_empty(&q_touchpad_stats))
    {
        queue_try_remove(&q_touchpad_stats, &stats);
    }

    bool active_game_buttons_map[NUM_GAME_BUTTONS] = {false};
    for (int i = 0; i < num_touch_sensors; i++)
    {
        if (stats.by_sensor[i].is_above_threshold())
        {
            active_game_buttons_map[touch_sensor_configs[i].button] = true;
        }
    }

    memset(hid_report_keycodes, 0, sizeof(hid_report_keycodes));
    int hid_keycode_idx = 0;
    for (int gbtn = 0; gbtn < NUM_GAME_BUTTONS; gbtn++)
    {
        if (active_game_buttons_map[gbtn])
        {
            uint8_t kc = game_button_to_keycode_map[gbtn];
            hid_report_keycodes[hid_keycode_idx] = kc;
            hid_keycode_idx++;
        }
    }
    hid_report_dirty = true;

    log_stats(stats);
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
    blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
    blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
    blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
    blink_interval_ms = BLINK_MOUNTED;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

void hid_task(void)
{
    // Remote wakeup
    if (tud_suspended())
    {
        // Wake up host if we are in suspend mode
        // and REMOTE_WAKEUP feature is enabled by host
        tud_remote_wakeup();
    }

    if (!hid_report_dirty) {
        return;
    }

    // skip if hid is not ready yet
    if (!tud_hid_ready())
        return;

    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, hid_report_keycodes);
    hid_report_dirty = false;
}

// // Invoked when sent REPORT successfully to host
// // Application can use this to send the next report
// // Note: For composite reports, report[0] is report ID
// void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len)
// {
//     (void)instance;
//     (void)len;

//     uint8_t next_report_id = report[0] + 1;

//     if (next_report_id < REPORT_ID_COUNT)
//     {
//         send_hid_report(next_report_id, board_button_read());
//     }
// }

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    // TODO not Implemented
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance;
    // do nothing
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
