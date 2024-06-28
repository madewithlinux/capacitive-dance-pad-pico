#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/timer.h"

#include "touch.pio.h"
#include "running_stats.hpp"
#include "touch_sensor_config.hpp"
#include "touch_sensor_thread.hpp"
#include "config.h"

#pragma region sensor config

// NOTE: THIS IS MUTABLE
uint16_t touch_sensor_thresholds[num_touch_sensors] = {200, 200, 200, 200, 200, 200, 200, 200};

#pragma endregion sensor config

static PIO pios[NUM_PIOS] = {pio0, pio1};
// NUM_PIO_STATE_MACHINES

void init_touch_sensors()
{
    uint pio0_offset = pio_add_program(pio0, &touch_program);
    IF_SERIAL_LOG(printf("Loaded program in pio0 at %d\n", pio0_offset));
    uint pio1_offset = pio_add_program(pio1, &touch_program);
    IF_SERIAL_LOG(printf("Loaded program in pio1 at %d\n", pio1_offset));

    const uint pio_offsets[NUM_PIOS] = {pio0_offset, pio1_offset};

    for (int i = 0; i < num_touch_sensors; i++)
    {
        touch_sensor_config_t cfg = touch_sensor_configs[i];
        const PIO pio = pios[cfg.pio_idx];
        const uint offset = pio_offsets[cfg.pio_idx];

        gpio_disable_pulls(cfg.pin);
        gpio_set_drive_strength(cfg.pin, GPIO_DRIVE_STRENGTH_12MA);

        touch_program_init(pio, cfg.sm, offset, cfg.pin);
        pio_sm_set_enabled(pio, cfg.sm, true);

        pio_set_irq0_source_enabled(
            pio, (enum pio_interrupt_source)((uint)pis_interrupt0 + cfg.sm), false);
        pio_set_irq1_source_enabled(
            pio, (enum pio_interrupt_source)((uint)pis_interrupt0 + cfg.sm), false);
    }
}

touchpad_stats_t sample_touch_inputs_for_us(uint64_t duration_us)
{
    uint64_t end_time = time_us_64() + duration_us;

    // just allocate all of them and assume that's ok
    running_stats stats_by_pio_sm[NUM_PIOS][NUM_PIO_STATE_MACHINES];
    // set the proper threshold values
    for (int i = 0; i < num_touch_sensors; i++)
    {
        touch_sensor_config_t cfg = touch_sensor_configs[i];
        stats_by_pio_sm[cfg.pio_idx][cfg.sm].threshold = touch_sensor_thresholds[i];
    }

    while (time_us_64() < end_time)
    {
        for (int pio_idx = 0; pio_idx < NUM_PIOS; pio_idx++)
        {
            const PIO pio = pios[pio_idx];

            pio_interrupt_clear(pio, 1);
            // for (uint sm = 0; sm < 4; sm++)
            for (int i = 0; i < num_touch_sensors / 2; i++)
            {
                touch_sensor_config_t cfg = touch_sensors_by_pio[pio_idx][i];
                int16_t value = TOUCH_TIMEOUT - pio_sm_get_blocking(pio, cfg.sm);
                stats_by_pio_sm[cfg.pio_idx][cfg.sm].add_value(value);
            }
            pio_interrupt_clear(pio, 0);
        }
    }
    std::array<running_stats, num_touch_sensors> by_sensor;
    for (int i = 0; i < num_touch_sensors; i++)
    {
        touch_sensor_config_t cfg = touch_sensor_configs[i];
        by_sensor[i] = stats_by_pio_sm[cfg.pio_idx][cfg.sm];
    }
    return {by_sensor};
}

void run_touch_sensor_thread()
{
    sleep_ms(2000);
    init_touch_sensors();

    IF_SERIAL_LOG(printf("pre-sample threshold values for all touch sensors\n"));
    // TODO: change LED blink rate to show status
    touchpad_stats_t stats = sample_touch_inputs_for_us(threshold_sampling_duration_us);
    for (int i = 0; i < num_touch_sensors; i++)
    {
        touch_sensor_thresholds[i] = uint16_t(stats.by_sensor[i].get_mean_float() * threshold_factor);
    }

    IF_SERIAL_LOG(printf("begin reading all 8 PIO touch values\n"));
    absolute_time_t last_timestamp0 = get_absolute_time();
    absolute_time_t last_timestamp1 = get_absolute_time();
    while (true)
    {
        touchpad_stats_t stats = sample_touch_inputs_for_us(sampling_duration_us);

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
                   touch_sensor_thresholds[7]));

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
        printf("%6.0f %6.0f %6.0f %6.0f||%6.0f %6.0f %6.0f %6.0f | %d, %5.0f Hz\n",
               stats.by_sensor[0].get_mean_float(),
               stats.by_sensor[1].get_mean_float(),
               stats.by_sensor[2].get_mean_float(),
               stats.by_sensor[3].get_mean_float(),
               stats.by_sensor[4].get_mean_float(),
               stats.by_sensor[5].get_mean_float(),
               stats.by_sensor[6].get_mean_float(),
               stats.by_sensor[7].get_mean_float(),
               reporting_time_us,
               reporting_freq);
        puts("\n");

        last_timestamp0 = last_timestamp1;
        last_timestamp1 = get_absolute_time();
    }
}
