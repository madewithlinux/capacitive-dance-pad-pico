#pragma once

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

constexpr uint num_touch_sensors = count_of(touch_sensor_configs);

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

#define TOUCH_LAYOUT_TYPE TOUCH_LAYOUT_ITG
// #define TOUCH_POLLING_TYPE TOUCH_POLLING_PARALLEL
#define TOUCH_POLLING_TYPE TOUCH_POLLING_SEQUENTIAL
