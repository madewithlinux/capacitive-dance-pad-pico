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

#include "touch.pio.h"

int64_t alarm_callback(alarm_id_t id, void *user_data)
{
    // Put your timeout handler code in here
    return 0;
}

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum
{
    BLINK_NOT_MOUNTED = 250,
    BLINK_MOUNTED = 1000,
    BLINK_SUSPENDED = 2500,

    BLINK_ALWAYS_ON = UINT32_MAX,
    BLINK_ALWAYS_OFF = 0
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);

int main()
{
    board_init();
    tud_init(BOARD_TUD_RHPORT);
    stdio_init_all();

    multicore_launch_core1(run_touch_sensor_thread);
    while (1)
    {
        tud_task(); // tinyusb device task
        // cdc_task();
        // webserial_task();
        led_blinking_task();
    }

    // // Timer example code - This example fires off the callback after 2000ms
    // add_alarm_in_ms(2000, alarm_callback, NULL, false);
    // // For more examples of timer use see https://github.com/raspberrypi/pico-examples/tree/master/timer

    // while (true)
    // {
    //     printf("Hello, world!\n");
    //     sleep_ms(1000);
    // }
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
    static uint32_t start_ms = 0;
    static bool led_state = false;

    // Blink every interval ms
    if (board_millis() - start_ms < blink_interval_ms)
        return; // not enough time
    start_ms += blink_interval_ms;

    board_led_write(led_state);
    led_state = 1 - led_state; // toggle
}
