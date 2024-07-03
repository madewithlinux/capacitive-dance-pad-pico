#pragma once
#include <array>
#include "running_stats.hpp"
#include "touch_sensor_config.hpp"

extern uint16_t touch_sensor_thresholds[num_touch_sensors];

struct touchpad_stats_t {
  const std::array<running_stats, num_touch_sensors> by_sensor;
};

// constexpr float threshold_factor = 1.5;
// constexpr uint64_t threshold_sampling_duration_us = 2 * 1000 * 1000;
// // constexpr uint64_t sampling_duration_us = 200 * 1000;
// constexpr uint64_t sampling_duration_us = 1 * 1000;

void __time_critical_func(run_touch_sensor_thread)();
