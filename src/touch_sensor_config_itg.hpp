#pragma once

constexpr touch_sensor_config_t touch_sensor_configs[] = {
    // clang-format off
    // - A -
    // C - B
    // - D -
    // btn   sensor   pin   pio  sm
    {UP    , /* A ,*/   7 ,   0,  0},
    {RIGHT , /* B ,*/   9 ,   0,  0},
    {DOWN  , /* C ,*/   8 ,   0,  0},
    {LEFT  , /* D ,*/   6 ,   0,  0},
    {START ,            5 ,   0,  0},
    {SELECT,            4 ,   0,  0},
    {BACK  ,            3 ,   0,  0},
    // clang-format on
};

constexpr uint num_touch_sensors = count_of(touch_sensor_configs);

#define TOUCH_LAYOUT_TYPE TOUCH_LAYOUT_ITG
#define TOUCH_POLLING_TYPE TOUCH_POLLING_SEQUENTIAL
