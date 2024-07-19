#include <charconv>
#include <string>
#include <variant>

#include "hardware/flash.h"
#include "pico/stdlib.h"

#include "tusb.h"

#include "config_defines.h"
#include "custom_logging.hpp"
#include "serial_config_console.hpp"
#include "touch_sensor_config.hpp"
#include "touch_sensor_thread.hpp"

#ifndef DEFAULT_THRESHOLD_FACTOR
#define DEFAULT_THRESHOLD_FACTOR 1.5
#endif  // DEFAULT_THRESHOLD_FACTOR
#ifndef DEFAULT_THRESHOLD_VALUE
#define DEFAULT_THRESHOLD_VALUE 150.0
#endif  // DEFAULT_THRESHOLD_VALUE

float threshold_factor = DEFAULT_THRESHOLD_FACTOR;
float threshold_value = DEFAULT_THRESHOLD_VALUE;
int threshold_type = THRESHOLD_TYPE_VALUE;
uint64_t threshold_sampling_duration_us = 2 * 1000 * 1000;
uint64_t sampling_duration_us = 1 * 1000;
uint64_t serial_teleplot_report_interval_us = 80 * 1000;
bool teleplot_normalize_values = true;
int filter_type = FILTER_TYPE_MEDIAN;
bool usb_hid_enabled = true;
float iir_filter_b = 0.8;

static config_console_value config_values[] = {
    {"threshold_factor", &threshold_factor},
    {"threshold_value", &threshold_value},
    {"threshold_type", &threshold_type},
    {"threshold_sampling_duration_us", &threshold_sampling_duration_us},
    {"sampling_duration_us", &sampling_duration_us},
    {"serial_teleplot_report_interval_us", &serial_teleplot_report_interval_us},
    {"teleplot_normalize_values", &teleplot_normalize_values},
    {"usb_hid_enabled", &usb_hid_enabled},
    {"filter_type", &filter_type},
    {"iir_filter_b", &iir_filter_b},
};

// #if SERIAL_CONFIG_CONSOLE

void erase_saved_config_in_flash();
bool write_config_to_flash();
bool read_config_from_flash();

void serial_console_task() {
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
      for (uint i = 0; i < count_of(config_values); i++) {
        config_values[i].print_config_line(itf);
      }
      tud_cdc_n_write_str(itf, "\r\n");
    } else if (line_buf.rfind("touch_sensor_thresholds") == 0) {
      for (uint i = 0; i < num_touch_sensors; i++) {
        CDC_PRINTF(itf, "touch_sensor_thresholds[%i] = %i\r\n", i, touch_sensor_thresholds[i]);
      }
    } else if (line_buf.rfind("set ") == 0) {
      char name_buf[128] = {0};
      char value_buf[128] = {0};
      sscanf(line_buf.c_str(), "set %128s %128s", name_buf, value_buf);
      std::string value_str(value_buf);

      bool found = false;
      for (uint i = 0; i < count_of(config_values); i++) {
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
    } else if (line_buf.rfind("save") == 0) {
      if (write_config_to_flash()) {
        CDC_PUTS(itf, "success");
      } else {
        CDC_PUTS(itf, "fail");
      }
    } else if (line_buf.rfind("load") == 0) {
      if (read_config_from_flash()) {
        CDC_PUTS(itf, "success");
      } else {
        CDC_PUTS(itf, "fail");
      }
    } else if (line_buf.rfind("reset") == 0) {
      erase_saved_config_in_flash();
      CDC_PUTS(itf, "unplug and replug to complete the config reset");
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
    uint64_t value_uint64_t2;
    fcr = std::from_chars(first, last, value_uint64_t2);
    if (fcr.ec == std::errc{}) {
      *value_uint64_t = value_uint64_t2;
    }
  } else if (value_int) {
    int value_int2;
    fcr = std::from_chars(first, last, value_int2);
    if (fcr.ec == std::errc{}) {
      *value_int = value_int2;
    }
  } else if (value_bool) {
    // TODO: better/easier bool format?
    int value_bool2;
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

const bool config_console_value::read_from_raw(const void* raw_value_ptr) {
  if (0) {
    // clang-format off
  } else if (value_float   ) { *value_float    = *((float    *) raw_value_ptr);
  } else if (value_uint64_t) { *value_uint64_t = *((uint64_t *) raw_value_ptr);
  } else if (value_int     ) { *value_int      = *((int      *) raw_value_ptr);
  } else if (value_bool    ) { *value_bool     = *((bool     *) raw_value_ptr);
    // clang-format on
  } else {
    // TODO: error handling?
    return false;
  }
  return true;
}

const bool config_console_value::write_to_raw(void* raw_value_ptr) {
  if (0) {
    // clang-format off
  } else if (value_float   ) {*((float    *) raw_value_ptr) = *value_float    ;
  } else if (value_uint64_t) {*((uint64_t *) raw_value_ptr) = *value_uint64_t ;
  } else if (value_int     ) {*((int      *) raw_value_ptr) = *value_int      ;
  } else if (value_bool    ) {*((bool     *) raw_value_ptr) = *value_bool     ;
    // clang-format on
  } else {
    // TODO: error handling?
    return false;
  }
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

void serial_console_init() {
  // attempt to read the config from flash memory, and if that fails (likely due to a config schema change), overwrite
  // it with the new default config
  if (!read_config_from_flash()) {
    write_config_to_flash();
  }
}

constexpr uint64_t current_config_version = 1;

struct flash_config_header {
  uint64_t version;
  uint64_t num_config_values;
};

// constexpr size_t flash_page_size_words = FLASH_PAGE_SIZE / sizeof(uint64_t);
constexpr size_t max_flash_config_value_elements = (FLASH_PAGE_SIZE - sizeof(flash_config_header)) / sizeof(uint64_t);

struct flash_config {
  flash_config_header header;
  uint64_t config_value_elements[max_flash_config_value_elements];
};

#define FLASH_TARGET_OFFSET (256 * 1024)

const flash_config* flash_config_contents = (const flash_config*)(XIP_BASE + FLASH_TARGET_OFFSET);

void erase_saved_config_in_flash() {
  flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
}
bool write_config_to_flash() {
  flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);

  static_assert(sizeof(flash_config) == FLASH_PAGE_SIZE, "config size mismatch");
  static_assert(count_of(config_values) <= max_flash_config_value_elements, "too many config values!");

  flash_config config_to_write;
  config_to_write.header.version = current_config_version;
  config_to_write.header.num_config_values = count_of(config_values);
  for (size_t i = 0; i < count_of(config_values); i++) {
    void* raw_value_ptr = &config_to_write.config_value_elements[i];
    if (!config_values[i].write_to_raw(raw_value_ptr)) {
      return false;
    }
  }

  flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t*)(&config_to_write), sizeof(flash_config));
  return true;
}

bool read_config_from_flash() {
  if (flash_config_contents->header.version != current_config_version) {
    return false;
  } else if (flash_config_contents->header.num_config_values != count_of(config_values)) {
    return false;
  }

  for (size_t i = 0; i < MIN(count_of(config_values), flash_config_contents->header.num_config_values); i++) {
    const void* raw_value_ptr = &(flash_config_contents->config_value_elements[i]);
    if (!config_values[i].read_from_raw(raw_value_ptr)) {
      return false;
    }
  }
  return true;
}
