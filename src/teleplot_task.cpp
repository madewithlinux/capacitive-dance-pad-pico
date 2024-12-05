#include "tusb.h"

#include "config_defines.h"
#include "custom_logging.hpp"
#include "sensor_data_filter.hpp"
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

void teleplot_summarize_buffered_data();

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
      // if (teleplot_normalize_values) {
      //   val = stats.by_sensor[i].get_mean_float() - touch_sensor_baseline[i];
      // } else {
      //   val = stats.by_sensor[i].get_mean_float();
      // }

      // val = normalized_touch_sensor_data[i].get_current_value();
      val = touch_sensor_data[i].get_current_value();

      raw_sum = raw_sum + stats.by_sensor[i].get_mean_float();
      base_sum = base_sum + touch_sensor_baseline[i];
      teleplot_printf(">t%d,s:%u:%.3f\r\n", i, timestamp, val);
    }
    teleplot_printf(">cur,sum:%ju:%f\r\n", timestamp, (double)raw_sum);
    teleplot_printf(">base,sum:%ju:%f\r\n", timestamp, (double)base_sum);

    teleplot_summarize_buffered_data();

    teleplot_flush();
  }
#endif
}

constexpr size_t sensor_data_send_buffer_size = 8192;
sensor_data_send_buffer_value_type sensor_data_send_buffer[sensor_data_send_buffer_size];
size_t sensor_data_send_buffer_read_cursor = 0;
size_t sensor_data_send_buffer_write_cursor = 0;

void write_to_sensor_data_send_buffer(sensor_data_send_buffer_value_type data) {
  sensor_data_send_buffer_write_cursor = (sensor_data_send_buffer_write_cursor + 1) % sensor_data_send_buffer_size;
  sensor_data_send_buffer[sensor_data_send_buffer_write_cursor] = data;
}

// FIXME: for some reason, if you use this function, the touch sampling thread will crash after a few seconds. (oh well,
// it works for long enough to get some useful data, at least)
void teleplot_summarize_buffered_data() {
  // unfortunately we cannot send the full sensor buffer, as that is too much data too fast. instead, report summary
  // statistics (min, max, mean, etc)

  size_t read_cur = sensor_data_send_buffer_read_cursor;
  size_t write_cur = sensor_data_send_buffer_write_cursor;
  if (read_cur == write_cur) {
    return;
  }

  sensor_data_send_buffer_value_type buf_min = INT16_MAX;
  sensor_data_send_buffer_value_type buf_max = INT16_MIN;
  int64_t buf_sum = 0;
  size_t buf_count = 0;
  for (size_t i = read_cur;                        //
       i != write_cur;                             //
       i = (i + 1) % sensor_data_send_buffer_size  //
  ) {
    buf_min = std::min(buf_min, sensor_data_send_buffer[i]);
    buf_max = std::max(buf_max, sensor_data_send_buffer[i]);
    buf_sum = buf_sum + sensor_data_send_buffer[i];
    buf_count++;
  }
  float buf_mean = ((float)buf_sum) / ((float)buf_count);
  teleplot_printf(">min,d:%d\r\n", buf_min);
  teleplot_printf(">max,d:%d\r\n", buf_max);
  teleplot_printf(">mean,d:%.3f\r\n", buf_mean);
  teleplot_printf(">buf_count:%d\r\n", buf_count);
  // teleplot_printf(">buf_mean:%.3f\r\n", buf_mean);
  teleplot_printf(">buf_sum:%d\r\n", buf_sum);

  // (void)buf_min;
  // (void)buf_max;
  // (void)buf_count;
  // (void)buf_mean;
  // (void)buf_sum;
  // buf_min = -1;
  // buf_max = 1;
  // buf_count = 2981.0000;
  // buf_mean = -0.0020;
  // buf_sum = -5.0;
  // teleplot_printf(">min,d:%d\r\n", buf_min);
  // teleplot_printf(">max,d:%d\r\n", buf_max);
  // teleplot_printf(">buf_count:%d\r\n", buf_count);
  // teleplot_printf(">mean:%.3f\r\n", buf_mean);
  // teleplot_printf(">buf_sum:%d\r\n", buf_sum);

  sensor_data_send_buffer_read_cursor = write_cur;
}
