#pragma once

#include "tusb.h"

#include "config_defines.h"
#include "touch_sensor_config.hpp"
#include "touch_sensor_thread.hpp"

extern uint8_t game_button_to_keycode_map[NUM_GAME_BUTTONS];
extern uint8_t hid_report_keycodes[6];
extern bool hid_report_dirty;
extern bool active_game_buttons_map[NUM_GAME_BUTTONS];
extern touchpad_stats_t stats;


void touch_stats_handler_task();
void hid_task();
