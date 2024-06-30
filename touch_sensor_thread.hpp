#pragma once
#include <array>
#include "running_stats.hpp"
#include "touch_sensor_config.hpp"

// TODO: move these to touch_sensor_config.hpp?
constexpr touch_sensor_config_t touch_sensor_configs[] = {
    // clang-format off
    // - A B -
    // H - - C
    // G - - D
    // - F E -
    // btn   sensor   pin   pio  sm
    {UP    , /* A ,*/  14 ,   0,  3},
    {UP    , /* B ,*/  15 ,   1,  3},
    {RIGHT , /* C ,*/  13 ,   0,  2},
    {RIGHT , /* D ,*/  12 ,   1,  2},
    {DOWN  , /* E ,*/  11 ,   0,  1},
    {DOWN  , /* F ,*/  10 ,   1,  1},
    {LEFT  , /* G ,*/   8 ,   0,  0},
    {LEFT  , /* H ,*/   9 ,   1,  0},
    // clang-format on
};
// constexpr int num_touch_sensors = (sizeof(touch_sensor_configs) /
// sizeof(touch_sensor_configs[0]));
constexpr int num_touch_sensors = count_of(touch_sensor_configs);

// split up for convenience elsewhere
constexpr touch_sensor_config_t
    touch_sensors_by_pio[NUM_PIOS][num_touch_sensors / 2] = {
        // clang-format off
    {
        {UP    , /* A ,*/  14 ,   0,  3},
        {RIGHT , /* C ,*/  13 ,   0,  2},
        {DOWN  , /* E ,*/  11 ,   0,  1},
        {LEFT  , /* G ,*/   8 ,   0,  0},
    },
    {
        {UP    , /* B ,*/  15 ,   1,  3},
        {RIGHT , /* D ,*/  12 ,   1,  2},
        {DOWN  , /* F ,*/  10 ,   1,  1},
        {LEFT  , /* H ,*/   9 ,   1,  0},
    },
        // clang-format on
};

extern uint16_t touch_sensor_thresholds[num_touch_sensors];

struct touchpad_stats_t {
  const std::array<running_stats, num_touch_sensors> by_sensor;
};

constexpr float threshold_factor = 2.0;
constexpr uint64_t threshold_sampling_duration_us = 2 * 1000 * 1000;
// constexpr uint64_t sampling_duration_us = 200 * 1000;
constexpr uint64_t sampling_duration_us = 1 * 1000;

void __time_critical_func(run_touch_sensor_thread)();
