#include "tusb.h"

#include "config_defines.h"
#include "custom_logging.hpp"
#include "serial_config_console.hpp"
#include "touch_hid_tasks.hpp"
#include "touch_sensor_config.hpp"
#include "touch_sensor_thread.hpp"

#include "teleplot_task.hpp"

#ifndef TOUCH_LAYOUT_BUTTONS
#if TOUCH_LAYOUT_TYPE == TOUCH_LAYOUT_ITG
#define TOUCH_LAYOUT_BUTTONS {LEFT, DOWN, UP, RIGHT}
#elif TOUCH_LAYOUT_TYPE == TOUCH_LAYOUT_PUMP
#define TOUCH_LAYOUT_BUTTONS {DOWN_LEFT, UP_LEFT, MIDDLE, UP_RIGHT, DOWN_RIGHT}
#else
// for now this is just the rhythm horizon layout, plus start and select
#define TOUCH_LAYOUT_BUTTONS {DOWN_LEFT, LEFT, UP_LEFT, DOWN, MIDDLE, UP, UP_RIGHT, RIGHT, DOWN_RIGHT, START, SELECT}
#endif  // TOUCH_LAYOUT_TYPE
#endif  // TOUCH_LAYOUT_BUTTONS

void teleplot_task() {
#if SERIAL_TELEPLOT

  static uint64_t last_teleplot_report_us = time_us_64();
  static uint32_t current_touch_sample_count = 0;
  static uint32_t last_touch_sample_count = 0;
  if (time_us_64() - last_teleplot_report_us > serial_teleplot_report_interval_us) {
    last_teleplot_report_us += serial_teleplot_report_interval_us;
    last_touch_sample_count = current_touch_sample_count;
    current_touch_sample_count = touch_sample_count;
    if (!teleplot_is_connected()) {
      return;
    }

    uint64_t timestamp = time_us_64() / 1000;

    if (current_touch_sample_count > 0 && last_touch_sample_count > 0) {
      float touch_sample_rate =
          (current_touch_sample_count - last_touch_sample_count) / ((float)serial_teleplot_report_interval_us * 1.0e-6);
      teleplot_printf(">r:%.3f\r\n", touch_sample_rate);
    }

    teleplot_printf(">b:%u:- ", timestamp);
    for (game_button btn : TOUCH_LAYOUT_BUTTONS) {
      if (active_game_buttons_map[btn]) {
        teleplot_write_str(game_button_short_labels[btn]);
      } else {
        teleplot_write_str("--");
      }
      teleplot_write_str(" ");
    }
    teleplot_puts("-|t");

    float raw_sum = 0;
    float base_sum = 0;
    for (uint i = 0; i < num_touch_sensors; i++) {
      float val;
      if (teleplot_normalize_values) {
        val = stats.by_sensor[i].get_mean_float() - touch_sensor_baseline[i];
      } else {
        val = stats.by_sensor[i].get_mean_float();
      }
      raw_sum = raw_sum + stats.by_sensor[i].get_mean_float();
      base_sum = base_sum + touch_sensor_baseline[i];
      teleplot_printf(">t%d,s:%u:%.3f\r\n", i, timestamp, val);
    }
    teleplot_printf(">cur,sum:%ju:%f\r\n", timestamp, (double) raw_sum);
    teleplot_printf(">base,sum:%ju:%f\r\n", timestamp, (double) base_sum);

    teleplot_flush();
  }
#endif
}
