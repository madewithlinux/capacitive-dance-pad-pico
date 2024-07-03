#include <charconv>
#include <string>
#include <variant>
#include "tusb.h"

#include "config_defines.h"
#include "custom_logging.hpp"
#include "serial_config_console.hpp"
#include "touch_sensor_config.hpp"
#include "touch_sensor_thread.hpp"

float threshold_factor = 1.5;
uint64_t threshold_sampling_duration_us = 2 * 1000 * 1000;
uint64_t sampling_duration_us = 1 * 1000;
uint64_t serial_teleplot_report_interval_us = 20 * 1000;
int filter_type = FILTER_TYPE_MEDIAN;
bool usb_hid_enabled = true;
float iir_filter_b = 0.8;

static config_console_value config_values[] = {
    {"threshold_factor", &threshold_factor},
    {"threshold_sampling_duration_us", &threshold_sampling_duration_us},
    {"sampling_duration_us", &sampling_duration_us},
    {"serial_teleplot_report_interval_us", &serial_teleplot_report_interval_us},
    {"usb_hid_enabled", &usb_hid_enabled},
    {"filter_type", &filter_type},
    {"iir_filter_b", &iir_filter_b},
};

// #if SERIAL_CONFIG_CONSOLE
void serial_console_task(void) {
  constexpr uint8_t itf = SERIAL_CONFIG_CONSOLE_INTERFACE;
  static std::string line_buf;

  if (!CDC_IS_CONNECTED(itf)) {
    return;
  }

  if (read_line_into_string(itf, line_buf)) {
    // CDC_PRINTF(itf, "line: %s\r\n",
    //            line_buf.c_str());
    if (line_buf.empty()) {
      // do nothing
    } else if (line_buf.rfind("list") == 0) {
      CDC_PUTS(itf, "config values:");
      for (int i = 0; i < count_of(config_values); i++) {
        config_values[i].print_config_line(itf);
      }
      tud_cdc_n_write_str(itf, "\r\n");
    } else if (line_buf.rfind("touch_sensor_thresholds") == 0) {
      for (int i = 0; i < num_touch_sensors; i++) {
        CDC_PRINTF(itf, "touch_sensor_thresholds[%i] = %i\r\n", i, touch_sensor_thresholds[i]);
      }
    } else if (line_buf.rfind("set ") == 0) {
      char name_buf[128] = {0};
      char value_buf[128] = {0};
      sscanf(line_buf.c_str(), "set %128s %128s", name_buf, value_buf);
      std::string value_str(value_buf);

      bool found = false;
      for (int i = 0; i < count_of(config_values); i++) {
        if (config_values[i].name == name_buf) {
          config_values[i].read_str(itf, value_str);
          found = true;
          break;
        }
      }
      if (!found) {
        // CDC_PUTS(itf, "config value not found");
        CDC_PRINTF(itf, "config value not found: %s\r\n", name_buf);
      }
    } else {
      CDC_PRINTF(itf, "command not recognized: %s\r\n", line_buf.c_str());
    }

    // clear the line to reset state
    line_buf.clear();
    // re-print the command prompt
    tud_cdc_n_write_str(itf, "> ");
    CDC_FLUSH(itf);
  }
}

// #else
// inline void serial_console_task(void);
// #endif

const void config_console_value::print_config_line(uint8_t itf) {
  CDC_PRINTF(itf, "-- %-40s: ", name.c_str());
  if (0) {
  } else if (value_float) {
    CDC_PRINTF(itf, "(float)    %f\r\n", *value_float);
  } else if (value_uint64_t) {
    CDC_PRINTF(itf, "(uint64_t) %llu\r\n", *value_uint64_t);
  } else if (value_int) {
    CDC_PRINTF(itf, "(int)      %d\r\n", *value_int);
  } else if (value_bool) {
    CDC_PRINTF(itf, "(bool)     %d\r\n", *value_bool);
  } else {
    CDC_PUTS(itf, "<invalid type>");
  }
  CDC_FLUSH(itf);
}

const bool config_console_value::read_str(uint8_t itf, const std::string& value_str) {
  const char* first = value_str.c_str();
  const char* last = value_str.c_str() + value_str.size();
  std::from_chars_result fcr;
  if (0) {
  } else if (value_float) {
    float value_float2;
    fcr = std::from_chars(first, last, value_float2);
    if (fcr.ec == std::errc{}) {
      *value_float = value_float2;
    }
  } else if (value_uint64_t) {
    float value_uint64_t2;
    fcr = std::from_chars(first, last, value_uint64_t2);
    if (fcr.ec == std::errc{}) {
      *value_uint64_t = value_uint64_t2;
    }
  } else if (value_int) {
    float value_int2;
    fcr = std::from_chars(first, last, value_int2);
    if (fcr.ec == std::errc{}) {
      *value_int = value_int2;
    }
  } else if (value_bool) {
    float value_bool2;
    fcr = std::from_chars(first, last, value_bool2);
    if (fcr.ec == std::errc{}) {
      *value_bool = value_bool2;
    }
  } else {
    CDC_PUTS(itf, "<invalid type>");
  }
  if (fcr.ec != std::errc{}) {
    CDC_PRINTF(itf, "error: %d\r\n", fcr.ec);
    CDC_FLUSH(itf);
    return false;
  }
  CDC_FLUSH(itf);
  return true;
}

/**
 * when return value is true, line_buf contains line that was read (without
 * trailing '\n' or '\r')
 */
bool read_line_into_string(uint8_t itf, std::string& line_buf, bool echo_mid_line) {
  if (tud_cdc_n_available(itf)) {
    while (true) {
      int32_t ich = tud_cdc_n_read_char(itf);

      if (ich == -1) {
        break;
      }
      // CDC_PRINTF(itf, "char: %d %c\r\n", ich, ich);
      if (echo_mid_line) {
        tud_cdc_n_write_char(itf, ich);
        tud_cdc_n_write_flush(itf);
      }
      if (ich == '\r') {
        tud_cdc_n_write_char(itf, '\n');
        tud_cdc_n_write_flush(itf);
        return true;
      } else if (ich == '\n') {
        return true;
      }

      line_buf.push_back(ich);
    }
  }
  return false;
}
