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
bool active_game_buttons_map[NUM_GAME_BUTTONS] = {false};

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

  // skip if hid is not ready yet
  if (!tud_hid_ready())
    return;

  memset(active_game_buttons_map, 0, sizeof(active_game_buttons_map));
  for (uint i = 0; i < num_touch_sensors; i++) {
    active_game_buttons_map[touch_sensor_configs[i].button] = touch_sensor_data[i].get_is_pressed();
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

  tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, hid_report_keycodes);
}
