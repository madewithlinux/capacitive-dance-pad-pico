#pragma once
#include <string>

bool read_line_into_string(uint8_t itf, std::string& line_buf, bool echo_mid_line = true);

void serial_console_task(void);

// config values:
extern float threshold_factor;
extern uint64_t threshold_sampling_duration_us;
extern uint64_t sampling_duration_us;
extern uint64_t serial_teleplot_report_interval_us;
extern bool usb_hid_enabled;

struct config_console_value {
  const std::string name;
  float* value_float = nullptr;
  uint64_t* value_uint64_t = nullptr;
  int* value_int = nullptr;
  bool* value_bool = nullptr;

 public:
  config_console_value(const std::string name, float* value_float) : name(name), value_float(value_float) {}
  config_console_value(const std::string name, uint64_t* value_uint64_t) : name(name), value_uint64_t(value_uint64_t) {}
  config_console_value(const std::string name, int* value_int) : name(name), value_int(value_int) {}
  config_console_value(const std::string name, bool* value_bool) : name(name), value_bool(value_bool) {}

  const void print_config_line(uint8_t itf);
  const bool read_str(uint8_t itf, const std::string& value_str);
};
