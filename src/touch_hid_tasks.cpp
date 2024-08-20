#include "multicore_ipc.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "tusb.h"
#include "usb_descriptors.h"

#include "touch_hid_tasks.hpp"

// TODO: some way to make these dependent on which player it is
uint8_t game_button_to_keycode_map[NUM_GAME_BUTTONS] = {
#if PLAYER_NUMBER == 1
    // DDR/ITG
    [UP] = HID_KEY_ARROW_UP,
    [DOWN] = HID_KEY_ARROW_DOWN,
    [LEFT] = HID_KEY_ARROW_LEFT,
    [RIGHT] = HID_KEY_ARROW_RIGHT,
    // pump
    [MIDDLE] = HID_KEY_S,
    [UP_LEFT] = HID_KEY_Q,
    [UP_RIGHT] = HID_KEY_E,
    [DOWN_LEFT] = HID_KEY_Z,
    [DOWN_RIGHT] = HID_KEY_C,
    // menu
    [START] = HID_KEY_ENTER,
    [SELECT] = HID_KEY_SLASH,
    [BACK] = HID_KEY_ESCAPE,
#elif PLAYER_NUMBER == 2
    // DDR/ITG
    [UP] = HID_KEY_KEYPAD_8,
    [DOWN] = HID_KEY_KEYPAD_2,
    [LEFT] = HID_KEY_KEYPAD_4,
    [RIGHT] = HID_KEY_KEYPAD_6,
    // pump
    [MIDDLE] = HID_KEY_KEYPAD_5,
    [UP_LEFT] = HID_KEY_KEYPAD_7,
    [UP_RIGHT] = HID_KEY_KEYPAD_9,
    [DOWN_LEFT] = HID_KEY_KEYPAD_1,
    [DOWN_RIGHT] = HID_KEY_KEYPAD_3,
    // menu
    [START] = HID_KEY_KEYPAD_ENTER,
    [SELECT] = HID_KEY_KEYPAD_0,
    [BACK] = HID_KEY_BACKSLASH,
#else
#error "invalid PLAYER_NUMBER"
#endif
};

uint8_t hid_report_keycodes[6] = {0};
bool hid_report_dirty = true;
bool active_game_buttons_map[NUM_GAME_BUTTONS] = {false};
touchpad_stats_t stats;

// for debounce
static uint64_t game_button_press_timestamp[NUM_GAME_BUTTONS] = {0};
static uint64_t game_button_release_timestamp[NUM_GAME_BUTTONS] = {0};

void touch_stats_handler_task() {
  if (queue_is_empty(&q_touchpad_stats)) {
    return;
  }

  // remove from the queue until it's empty, because we want the most recent one
  while (!queue_is_empty(&q_touchpad_stats)) {
    queue_try_remove(&q_touchpad_stats, &stats);
  }

  bool prev_active_game_buttons_map[NUM_GAME_BUTTONS] = {0};
  memcpy(prev_active_game_buttons_map, active_game_buttons_map, sizeof(active_game_buttons_map));
  // (void) prev_active_game_buttons_map;
  // (void) game_button_press_timestamp;
  // (void) game_button_release_timestamp;

  memset(active_game_buttons_map, 0, sizeof(active_game_buttons_map));
  for (uint i = 0; i < num_touch_sensors; i++) {
    switch (filter_type) {
      case FILTER_TYPE_MEDIAN:
        if (stats.by_sensor[i].median_is_above_threshold()) {
          active_game_buttons_map[touch_sensor_configs[i].button] = true;
        }
        break;
      case FILTER_TYPE_AVG:
        if (stats.by_sensor[i].avg_is_above_threshold()) {
          active_game_buttons_map[touch_sensor_configs[i].button] = true;
        }
        break;
      case FILTER_TYPE_IIR:
        if (stats.by_sensor[i].iir_is_above_threshold()) {
          active_game_buttons_map[touch_sensor_configs[i].button] = true;
        }
        break;

      default:
        // TODO invalid value
        break;
    }
  }

  if (debounce_us) {
    uint64_t now_us = time_us_64();
    for (int gbtn = 0; gbtn < NUM_GAME_BUTTONS; gbtn++) {
      if (now_us - game_button_press_timestamp[gbtn] < debounce_us) {
        // inside press debounce window
        // ignore that release
        active_game_buttons_map[gbtn] = true;
      } else if (now_us - game_button_release_timestamp[gbtn] < debounce_us) {
        // inside release debounce window
        // ignore that press
        active_game_buttons_map[gbtn] = false;
      } else if (prev_active_game_buttons_map[gbtn] && !active_game_buttons_map[gbtn]) {
        // was pressed, now released
        game_button_release_timestamp[gbtn] = now_us;
      } else if (!prev_active_game_buttons_map[gbtn] && active_game_buttons_map[gbtn]) {
        // was released, now pressed
        game_button_press_timestamp[gbtn] = now_us;
      }
    }
  }

  memset(hid_report_keycodes, 0, sizeof(hid_report_keycodes));
  int hid_keycode_idx = 0;
  for (int gbtn = 0; gbtn < NUM_GAME_BUTTONS; gbtn++) {
    if (active_game_buttons_map[gbtn]) {
      uint8_t kc = game_button_to_keycode_map[gbtn];
      hid_report_keycodes[hid_keycode_idx] = kc;
      hid_keycode_idx++;
    }
  }
  hid_report_dirty = true;
}

void hid_task(void) {
  if (!usb_hid_enabled) {
    return;
  }
  // Remote wakeup
  if (tud_suspended()) {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  }

  if (!hid_report_dirty) {
    return;
  }

  // skip if hid is not ready yet
  if (!tud_hid_ready())
    return;

  tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, hid_report_keycodes);
  hid_report_dirty = false;
}
