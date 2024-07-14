#pragma once
#include "hardware/pio.h"

enum game_button {
  UP,
  DOWN,
  LEFT,
  RIGHT,
  // additional buttons used by pump and smx
  MIDDLE,
  UP_LEFT,
  UP_RIGHT,
  DOWN_LEFT,
  DOWN_RIGHT,
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

// values for TOUCH_POLLING_TYPE
#define TOUCH_POLLING_PARALLEL 1
#define TOUCH_POLLING_SEQUENTIAL 2

// values for TOUCH_LAYOUT_TYPE
#define TOUCH_LAYOUT_ITG 1
#define TOUCH_LAYOUT_DDR TOUCH_LAYOUT_ITG
#define TOUCH_LAYOUT_PUMP 2

// TODO: make this a config define thing, to enable multiple different builds
// #include "touch_sensor_config_itg8.hpp"
#include "touch_sensor_config_pump.hpp"

#ifndef TOUCH_POLLING_TYPE
#define TOUCH_POLLING_TYPE TOUCH_POLLING_PARALLEL
#endif // TOUCH_POLLING_TYPE
