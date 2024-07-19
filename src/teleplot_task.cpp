#include "tusb.h"

#include "config_defines.h"
#include "custom_logging.hpp"
#include "serial_config_console.hpp"
#include "touch_hid_tasks.hpp"
#include "touch_sensor_config.hpp"
#include "touch_sensor_thread.hpp"

#include "teleplot_task.hpp"

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

#if TOUCH_LAYOUT_TYPE == TOUCH_LAYOUT_ITG

    teleplot_printf(">b:%u:- ", timestamp);
    for (game_button btn : {LEFT, DOWN, UP, RIGHT}) {
      if (active_game_buttons_map[btn]) {
        teleplot_write_str(game_button_short_labels[btn]);
      } else {
        teleplot_write_str("--");
      }
      teleplot_write_str(" ");
    }
    teleplot_puts("-|t");

#elif TOUCH_LAYOUT_TYPE == TOUCH_LAYOUT_PUMP
    teleplot_puts(active_game_buttons_map[DOWN_LEFT] ? ">DOWN_LEFT:DOWN_LEFT|t" : ">DOWN_LEFT:.|t");
    teleplot_puts(active_game_buttons_map[UP_LEFT] ? ">UP_LEFT:UP_LEFT|t" : ">UP_LEFT:.|t");
    teleplot_puts(active_game_buttons_map[MIDDLE] ? ">MIDDLE:MIDDLE|t" : ">MIDDLE:.|t");
    teleplot_puts(active_game_buttons_map[UP_RIGHT] ? ">UP_RIGHT:UP_RIGHT|t" : ">UP_RIGHT:.|t");
    teleplot_puts(active_game_buttons_map[DOWN_RIGHT] ? ">DOWN_RIGHT:DOWN_RIGHT|t" : ">DOWN_RIGHT:.|t");
#endif  // TOUCH_LAYOUT_TYPE

      for (uint i = 0; i < num_touch_sensors; i++) {
        float val;
        if (teleplot_normalize_values) {
          val = stats.by_sensor[i].get_mean_float() - touch_sensor_baseline[i];
        } else {
          val = stats.by_sensor[i].get_mean_float();
        }
        teleplot_printf(">t%d,s:%u:%.3f\r\n", i, timestamp, val);
      }

    teleplot_flush();
  }
#endif
}
