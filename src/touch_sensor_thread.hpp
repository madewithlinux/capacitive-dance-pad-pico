#pragma once
#include <array>
#include "touch_sensor_config.hpp"
#include "sensor_data_filter.hpp"

extern uint16_t touch_sensor_baseline[num_touch_sensors];
extern sensor_data_filter touch_sensor_data[num_touch_sensors];
extern size_t calibration_sample_count;

// for deriving the sampling rate
extern volatile uint32_t touch_sample_count;

void __time_critical_func(run_touch_sensor_thread)();
