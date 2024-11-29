#pragma once

constexpr touch_sensor_config_t touch_sensor_configs[] = {
    // clang-format off
    // A - B
    // - C -
    // D - E
    // btn         sensor   pin   pio  sm
    {UP_LEFT    , /* A ,*/   6 ,   0,  0},
    {UP_RIGHT   , /* B ,*/  10 ,   0,  0},
    {MIDDLE     , /* C ,*/   8 ,   0,  0},
    {DOWN_LEFT  , /* D ,*/   7 ,   0,  0},
    // DOWN_RIGHT should be pin 9, but that pin is apparently dead on one of my picos
    {DOWN_RIGHT , /* E ,*/   12 ,   0,  0},
    // clang-format on
};

constexpr int num_touch_sensors = count_of(touch_sensor_configs);

#define TOUCH_LAYOUT_TYPE TOUCH_LAYOUT_PUMP
#define TOUCH_POLLING_TYPE TOUCH_POLLING_SEQUENTIAL
