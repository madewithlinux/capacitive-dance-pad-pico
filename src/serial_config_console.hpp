#pragma once
#include <string>
#include "config_values.hpp"

bool read_line_into_string(uint8_t itf, std::string& line_buf, bool echo_mid_line = true);

void serial_console_init();

void serial_console_task();

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

  const bool read_from_raw(const void * raw_value_ptr);
  const bool write_to_raw(void * raw_value_ptr);
};
