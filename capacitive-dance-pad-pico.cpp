#include <stdio.h>
#include <vector>
#include <array>
#include <numeric>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/timer.h"

#include "blink.pio.h"
#include "touch.pio.h"

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq)
{
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    printf("Blinking pin %d at %d Hz\n", pin, freq);

    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    pio->txf[sm] = (125000000 / (2 * freq)) - 3;
}

void read_touch_sensor(PIO pio, uint sm, uint offset, uint pin)
{
    gpio_disable_pulls(pin);
    gpio_set_drive_strength(pin, GPIO_DRIVE_STRENGTH_12MA);

    touch_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);
    pio_set_irq0_source_enabled(pio, (enum pio_interrupt_source)((uint)pis_interrupt0 + sm), false);
    pio_set_irq1_source_enabled(pio, (enum pio_interrupt_source)((uint)pis_interrupt0 + sm), false);
    // pio_interrupt_clear(pio, 0);

    int32_t touch_timeout = 1 << 12;

    // sleep_ms(4000);
    // while (true)
    // {
    //     while (pio_sm_is_rx_fifo_empty(pio, sm))
    //     {
    //         tight_loop_contents();
    //     }
    //     int32_t value = touch_timeout - pio->rxf[sm];
    //     printf("capacitive read value: %u\n", value);
    //     sleep_ms(100);
    // }
    sleep_ms(2000);
    printf("begin reading PIO fifo\n");
    while (true)
    {
        pio_interrupt_clear(pio, 0);
        int32_t value = touch_timeout - pio_sm_get_blocking(pio, sm);
        // int32_t value = pio_sm_get_blocking(pio, sm);
        printf("capacitive read value: %u\n", value);
        sleep_ms(250);
    }
}

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

                pio_interrupt_clear(pio, 0);
                for (uint sm = 0; sm < 4; sm++)
                {
                    int16_t value = TOUCH_TIMEOUT - pio_sm_get_blocking(pio, sm);
                    buffer[buffer_offset + sm].push_back(value);
                }
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

int main()
{
    stdio_init_all();

    sleep_ms(4000);

    // // PIO Blinking example
    // PIO pio = pio0;
    // uint offset = pio_add_program(pio, &touch_program);
    // printf("Loaded program at %d\n", offset);

    // for (int i = 8; i < 15; i++)
    // {
    //     gpio_set_dir(i, true);
    //     gpio_put(i, false);
    // }
    // read_touch_sensor(pio, 0, offset, 15);

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

    // Timer example code - This example fires off the callback after 2000ms
    add_alarm_in_ms(2000, alarm_callback, NULL, false);
    // For more examples of timer use see https://github.com/raspberrypi/pico-examples/tree/master/timer

    while (true)
    {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
