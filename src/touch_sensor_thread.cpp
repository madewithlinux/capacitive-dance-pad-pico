#include <stdio.h>

#include "hardware/pio.h"
#include "hardware/timer.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "config_defines.h"
#include "multicore_ipc.h"
#include "serial_config_console.hpp"
#include "touch.pio.h"
#include "touch_sensor_config.hpp"
#include "touch_sensor_thread.hpp"

#pragma region sensor config

// NOTE: THESE ARE MUTABLE
uint16_t touch_sensor_baseline[num_touch_sensors] = {0};
sensor_data_filter touch_sensor_data[num_touch_sensors];
size_t calibration_sample_count = 0;

volatile uint32_t touch_sample_count = 0;

#pragma endregion sensor config

#if TOUCH_POLLING_TYPE == TOUCH_POLLING_PARALLEL
#error "this is no longer supported"
#elif TOUCH_POLLING_TYPE == TOUCH_POLLING_SEQUENTIAL

static uint pio0_offset;
void init_touch_sensors() {
  // for sequential polling, we just need one PIO
  pio0_offset = pio_add_program(pio0, &touch_program);
  IF_SERIAL_LOG(printf("Loaded program in pio0 at %d\n", pio0_offset));

  for (uint i = 0; i < num_touch_sensors; i++) {
    touch_sensor_config_t cfg = touch_sensor_configs[i];
    // ignore the config-specified PIO and state machine
    // assert(cfg.pio_idx == 0);
    // assert(cfg.sm == 0);

    gpio_disable_pulls(cfg.pin);
    gpio_set_drive_strength(cfg.pin, GPIO_DRIVE_STRENGTH_12MA);

    pio_sm_set_enabled(pio0, 0, false);
    touch_program_init(pio0, 0, pio0_offset, cfg.pin);
    pio_sm_set_enabled(pio0, 0, true);
    pio_set_irq0_source_enabled(pio0, (enum pio_interrupt_source)((uint)pis_interrupt0), false);
    pio_set_irq1_source_enabled(pio0, (enum pio_interrupt_source)((uint)pis_interrupt0), false);
  }
}

struct touchpad_stats_t {
  std::array<uint64_t, num_touch_sensors> sum_by_sensor = {0};
  size_t sample_count = 0;
};

touchpad_stats_t __time_critical_func(init_sample_touch_inputs_for_us)(uint64_t duration_us) {
  uint64_t end_time = time_us_64() + duration_us - sampling_buffer_time_us;

  touchpad_stats_t init_stats = {0};
  // including skipped samples
  size_t total_sample_count = 0;

  while (time_us_64() < end_time) {
    for (uint i = 0; i < num_touch_sensors; i++) {
      touch_sensor_config_t cfg = touch_sensor_configs[i];

      pio_interrupt_clear(pio0, 1);
      pio_sm_set_enabled(pio0, 0, false);
      touch_program_init(pio0, 0, pio0_offset, cfg.pin);
      pio_sm_set_enabled(pio0, 0, true);

      int16_t value = TOUCH_TIMEOUT - pio_sm_get_blocking(pio0, 0);
      if (total_sample_count > cfg_calibration_skip_samples) {
        init_stats.sum_by_sensor[i] += value;
      }
      pio_interrupt_clear(pio0, 0);
      // pio_interrupt_clear(pio0, 1);
      if (sleep_us_between_samples) {
        sleep_us(sleep_us_between_samples);
      }
    }
    if (total_sample_count > cfg_calibration_skip_samples) {
      init_stats.sample_count++;
    }
    total_sample_count++;
  }
  return init_stats;
}

void sample_touch_inputs_forever() {
  while (true) {
    for (uint i = 0; i < num_touch_sensors; i++) {
      touch_sensor_config_t cfg = touch_sensor_configs[i];

      pio_interrupt_clear(pio0, 1);
      pio_sm_set_enabled(pio0, 0, false);
      touch_program_init(pio0, 0, pio0_offset, cfg.pin);
      pio_sm_set_enabled(pio0, 0, true);

      int16_t value = TOUCH_TIMEOUT - pio_sm_get_blocking(pio0, 0);
      touch_sensor_data[i].update(value);
      pio_interrupt_clear(pio0, 0);
      // pio_interrupt_clear(pio0, 1);
      if (sleep_us_between_samples) {
        sleep_us(sleep_us_between_samples);
      }
    }
    touch_sample_count++;
  }
}

#endif  // TOUCH_POLLING_TYPE

void __time_critical_func(run_touch_sensor_thread)() {
  sleep_ms(250);
  blink_interval_t blink = BLINK_SENSORS_INIT;
  queue_add_blocking(&q_blink_interval, &blink);
  init_touch_sensors();

  IF_SERIAL_LOG(printf("pre-sample threshold values for all touch sensors\n"));
  blink = BLINK_SENSORS_CALIBRATING;
  queue_add_blocking(&q_blink_interval, &blink);
  touchpad_stats_t init_stats = init_sample_touch_inputs_for_us(threshold_sampling_duration_us);
  calibration_sample_count = init_stats.sample_count;
  for (uint i = 0; i < num_touch_sensors; i++) {
    touch_sensor_baseline[i] = ((float)init_stats.sum_by_sensor[i]) / ((float)init_stats.sample_count);
    touch_sensor_data[i] = sensor_data_filter(i, touch_sensor_baseline[i]);
  }

  IF_SERIAL_LOG(printf("begin reading all 8 PIO touch values\n"));
  blink = BLINK_SENSORS_OK;
  queue_add_blocking(&q_blink_interval, &blink);
  sample_touch_inputs_forever();
}
