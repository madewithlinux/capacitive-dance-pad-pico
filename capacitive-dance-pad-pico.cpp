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

#include "touch.pio.h"

PIO pios[NUM_PIOS] = {pio0, pio1};

void read_8_touch_sensors(const uint pio0_offset, const uint pio1_offset, const uint pins[8])
{
    const uint pio_offsets[NUM_PIOS] = {pio0_offset, pio1_offset};
    for (int i = 0; i < 8; i++)
    {
        const uint pin = pins[i];
        const PIO pio = pios[i / 4];
        const uint offset = pio_offsets[i / 4];
        const uint sm = i % 4;

        gpio_disable_pulls(pin);
        gpio_set_drive_strength(pin, GPIO_DRIVE_STRENGTH_12MA);
        touch_program_init(pio, sm, offset, pin);
        pio_sm_set_enabled(pio, sm, true);
        pio_set_irq0_source_enabled(
            pio, (enum pio_interrupt_source)((uint)pis_interrupt0 + sm), false);
        pio_set_irq1_source_enabled(
            pio, (enum pio_interrupt_source)((uint)pis_interrupt0 + sm), false);
    }

    // pio_interrupt_clear(pio, 0);

    const int num_samples = 8000;
    std::array<std::vector<int16_t>, 8> buffer;
    for (int i = 0; i < 8; i++)
    {
        buffer[i].reserve(num_samples);
    }

    // sleep_ms(2000);
    absolute_time_t last_timestamp0 = get_absolute_time();
    absolute_time_t last_timestamp1 = get_absolute_time();
    printf("begin reading all 8 PIO touch values\n");
    while (true)
    {
        absolute_time_t sampling_start_time = get_absolute_time();
        // collect a bunch of samples
        for (int sample_idx = 0; sample_idx < num_samples; sample_idx++)
        {
            for (int pio_index = 0; pio_index < 2; pio_index++)
            {
                const PIO pio = pios[pio_index];
                const PIO next_pio = pios[(pio_index + 1) % NUM_PIOS];
                const size_t buffer_offset = pio_index * 4;

                pio_interrupt_clear(pio, 1);
                for (uint sm = 0; sm < 4; sm++)
                {
                    int16_t value = TOUCH_TIMEOUT - pio_sm_get_blocking(pio, sm);
                    buffer[buffer_offset + sm].push_back(value);
                }
                pio_interrupt_clear(pio, 0);
            }
        }
        absolute_time_t sampling_end_time = get_absolute_time();
        int64_t sampling_time_us = absolute_time_diff_us(sampling_start_time, sampling_end_time);
        float sampling_freq = ((float)num_samples) / ((float)sampling_time_us / 1.0e6);

        // average all the samples
        std::array<float, 8> avg;
        for (int i = 0; i < 8; i++)
        {
            avg[i] = std::accumulate(buffer[i].begin(), buffer[i].end(), 0.0) / ((float)num_samples);
            buffer[i].clear();
        }

        int64_t reporting_time_us = absolute_time_diff_us(last_timestamp0, last_timestamp1);
        float reporting_freq = ((float)num_samples) / ((float)reporting_time_us / 1.0e6);

        printf("%6.0f %6.0f %6.0f %6.0f||%6.0f %6.0f %6.0f %6.0f | %d, %5.0f Hz;  %d, %5.0f Hz\n",
               avg[0],
               avg[1],
               avg[2],
               avg[3],
               avg[4],
               avg[5],
               avg[6],
               avg[7],
               sampling_time_us,
               sampling_freq,
               reporting_time_us,
               reporting_freq);

        last_timestamp0 = last_timestamp1;
        last_timestamp1 = get_absolute_time();
    }
}

int64_t alarm_callback(alarm_id_t id, void *user_data)
{
    // Put your timeout handler code in here
    return 0;
}

void core1_entry()
{
    sleep_ms(2000);
    uint pio0_offset = pio_add_program(pio0, &touch_program);
    printf("Loaded program in pio0 at %d\n", pio0_offset);
    uint pio1_offset = pio_add_program(pio1, &touch_program);
    printf("Loaded program in pio1 at %d\n", pio1_offset);
    // uint touch_pins[8] = {8, 9, 10, 11, 12, 13, 14, 15};

    uint touch_pins[8] = {
        // map adjacent pads to different PIO units
        // pio0
        8, 11, 13, 14,
        // pio1
        9, 10, 12, 15
        //
    };
    read_8_touch_sensors(pio0_offset, pio1_offset, touch_pins);
}

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED     = 1000,
  BLINK_SUSPENDED   = 2500,

  BLINK_ALWAYS_ON   = UINT32_MAX,
  BLINK_ALWAYS_OFF  = 0
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);

int main()
{
    board_init();
    tud_init(BOARD_TUD_RHPORT);
    stdio_init_all();

    multicore_launch_core1(core1_entry);
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
  if ( board_millis() - start_ms < blink_interval_ms) return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}
