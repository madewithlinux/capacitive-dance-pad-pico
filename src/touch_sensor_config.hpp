#pragma once
#include "hardware/pio.h"

enum game_button {
  UP,
  DOWN,
  LEFT,
  RIGHT,
  // additional buttons used by pump and smx
  MIDDLE,
  UP_RIGHT,
  UP_LEFT,
  DOWN_RIGHT,
  DOWN_LEFT,
  // menu buttons (probably also will never be used)
  START,
  SELECT,
  NUM_GAME_BUTTONS,
  INVALID = 0xff
};

struct touch_sensor_config_t {
  const game_button button;
  const uint8_t pin;
  const uint8_t pio_idx;
  const uint8_t sm;

  // constexpr touch_sensor_config_t() : button(INVALID), pin(0), pio_idx(0),
  // sm(0) {}

  constexpr touch_sensor_config_t(game_button button,
                                  uint8_t pin,
                                  uint8_t pio_idx,
                                  uint8_t sm)
      : button(button), pin(pin), pio_idx(pio_idx), sm(sm) {}
};

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
