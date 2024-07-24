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
  BACK,
  NUM_GAME_BUTTONS,
  INVALID = 0xff
};

// clang-format off
constexpr const char* game_button_short_labels[] = {
  [UP] = "UU",
  [DOWN] = "DD",
  [LEFT] = "LL",
  [RIGHT] = "RR",
  [MIDDLE] = "MI",
  [UP_LEFT] = "UL",
  [UP_RIGHT] = "UR",
  [DOWN_LEFT] = "DL",
  [DOWN_RIGHT] = "DR",
  [START] = "ST",
  [SELECT] = "SE",
  [BACK] = "BK",
  [NUM_GAME_BUTTONS] = "##",
};
// clang-format on

struct touch_sensor_config_t {
  const game_button button;
  const uint8_t pin;
  const uint8_t pio_idx;
  const uint8_t sm;

  // constexpr touch_sensor_config_t() : button(INVALID), pin(0), pio_idx(0),
  // sm(0) {}

  constexpr touch_sensor_config_t(game_button button, uint8_t pin, uint8_t pio_idx, uint8_t sm)
      : button(button), pin(pin), pio_idx(pio_idx), sm(sm) {}
};

// values for TOUCH_POLLING_TYPE
#define TOUCH_POLLING_PARALLEL 1
#define TOUCH_POLLING_SEQUENTIAL 2

// values for TOUCH_LAYOUT_TYPE
#define TOUCH_LAYOUT_ITG 1
#define TOUCH_LAYOUT_DDR TOUCH_LAYOUT_ITG
#define TOUCH_LAYOUT_PUMP 2

// values for TOUCH_SENSOR_CONFIG
#define TOUCH_SENSOR_CONFIG_ITG8 1
#define TOUCH_SENSOR_CONFIG_PUMP 2
#define TOUCH_SENSOR_CONFIG_ITG  3

// #ifndef TOUCH_SENSOR_CONFIG
// #error "config parameter not defined: TOUCH_SENSOR_CONFIG"
// #endif // TOUCH_SENSOR_CONFIG
#if TOUCH_SENSOR_CONFIG == TOUCH_SENSOR_CONFIG_ITG8
#include "touch_sensor_config_itg8.hpp"
#elif TOUCH_SENSOR_CONFIG == TOUCH_SENSOR_CONFIG_PUMP
#include "touch_sensor_config_pump.hpp"
#elif TOUCH_SENSOR_CONFIG == TOUCH_SENSOR_CONFIG_ITG
#include "touch_sensor_config_itg.hpp"
#else
#error "invalid value for TOUCH_SENSOR_CONFIG"
#endif  // TOUCH_SENSOR_CONFIG

#ifndef TOUCH_POLLING_TYPE
#define TOUCH_POLLING_TYPE TOUCH_POLLING_PARALLEL
#endif  // TOUCH_POLLING_TYPE
