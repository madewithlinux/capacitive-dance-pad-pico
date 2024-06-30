#include <stdio.h>

#include "hardware/pio.h"
#include "hardware/timer.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "config.h"
#include "multicore_ipc.h"
#include "running_stats.hpp"
#include "touch.pio.h"
#include "touch_sensor_config.hpp"
#include "touch_sensor_thread.hpp"

#pragma region sensor config

// NOTE: THIS IS MUTABLE
uint16_t touch_sensor_thresholds[num_touch_sensors] = {200, 200, 200, 200,
                                                       200, 200, 200, 200};

#pragma endregion sensor config

static PIO pios[NUM_PIOS] = {pio0, pio1};
// NUM_PIO_STATE_MACHINES

void init_touch_sensors() {
  uint pio0_offset = pio_add_program(pio0, &touch_program);
  IF_SERIAL_LOG(printf("Loaded program in pio0 at %d\n", pio0_offset));
  uint pio1_offset = pio_add_program(pio1, &touch_program);
  IF_SERIAL_LOG(printf("Loaded program in pio1 at %d\n", pio1_offset));

  const uint pio_offsets[NUM_PIOS] = {pio0_offset, pio1_offset};

  for (int i = 0; i < num_touch_sensors; i++) {
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

touchpad_stats_t __time_critical_func(sample_touch_inputs_for_us)(
    uint64_t duration_us) {
  uint64_t end_time = time_us_64() + duration_us;

  // just allocate all of them and assume that's ok
  running_stats stats_by_pio_sm[NUM_PIOS][NUM_PIO_STATE_MACHINES];
  // set the proper threshold values
  for (int i = 0; i < num_touch_sensors; i++) {
    touch_sensor_config_t cfg = touch_sensor_configs[i];
    stats_by_pio_sm[cfg.pio_idx][cfg.sm].threshold = touch_sensor_thresholds[i];
  }

  while (time_us_64() < end_time) {
    for (int pio_idx = 0; pio_idx < NUM_PIOS; pio_idx++) {
      const PIO pio = pios[pio_idx];

      pio_interrupt_clear(pio, 1);
      // for (uint sm = 0; sm < 4; sm++)
      for (int i = 0; i < num_touch_sensors / 2; i++) {
        touch_sensor_config_t cfg = touch_sensors_by_pio[pio_idx][i];
        int16_t value = TOUCH_TIMEOUT - pio_sm_get_blocking(pio, cfg.sm);
        stats_by_pio_sm[cfg.pio_idx][cfg.sm].add_value(value);
      }
      pio_interrupt_clear(pio, 0);
    }
  }
  std::array<running_stats, num_touch_sensors> by_sensor;
  for (int i = 0; i < num_touch_sensors; i++) {
    touch_sensor_config_t cfg = touch_sensor_configs[i];
    by_sensor[i] = stats_by_pio_sm[cfg.pio_idx][cfg.sm];
  }
  return {by_sensor};
}

void __time_critical_func(run_touch_sensor_thread)() {
  sleep_ms(2000);
  blink_interval_t blink = BLINK_SENSORS_INIT;
  queue_add_blocking(&q_blink_interval, &blink);
  init_touch_sensors();

  IF_SERIAL_LOG(printf("pre-sample threshold values for all touch sensors\n"));
  blink = BLINK_SENSORS_CALIBRATING;
  queue_add_blocking(&q_blink_interval, &blink);
  touchpad_stats_t stats =
      sample_touch_inputs_for_us(threshold_sampling_duration_us);
  for (int i = 0; i < num_touch_sensors; i++) {
    touch_sensor_thresholds[i] =
        uint16_t(stats.by_sensor[i].get_mean_float() * threshold_factor);
  }

  IF_SERIAL_LOG(printf("begin reading all 8 PIO touch values\n"));
  blink = BLINK_SENSORS_OK;
  queue_add_blocking(&q_blink_interval, &blink);
  while (true) {
    touchpad_stats_t stats = sample_touch_inputs_for_us(sampling_duration_us);
    if (queue_is_full(&q_touchpad_stats)) {
      touchpad_stats_t dummy;
      queue_remove_blocking(&q_touchpad_stats, &dummy);
    }
    queue_add_blocking(&q_touchpad_stats, &stats);
  }
}
