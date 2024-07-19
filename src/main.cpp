// #include <fmt/core.h>
// #include <fmt/format.h>

#include <stdio.h>
#include <array>
#include <numeric>
#include <vector>

#include "hardware/pio.h"
#include "hardware/timer.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "bsp/board.h"
#include "multicore_ipc.h"
#include "touch_sensor_config.hpp"
#include "touch_sensor_thread.hpp"
#include "tusb.h"
#include "usb_descriptors.h"

#include "config_defines.h"
#include "custom_logging.hpp"
#include "serial_config_console.hpp"
#include "teleplot_task.hpp"
#include "touch_hid_tasks.hpp"

#include "touch.pio.h"

static blink_interval_t blink_interval_ms = BLINK_INIT;


#define URL "example.tinyusb.org/webusb-serial/index.html"

const tusb_desc_webusb_url_t desc_url = {.bLength = 3 + sizeof(URL) - 1,
                                         .bDescriptorType = 3,  // WEBUSB URL type
                                         .bScheme = 1,          // 0: http, 1: https
                                         .url = URL};

static bool web_serial_connected = false;

void led_blinking_task(void);
void webserial_task(void);

int main() {
  board_init();
  tud_init(BOARD_TUD_RHPORT);
  stdio_init_all();
  init_queues();
  serial_console_init();

  multicore_launch_core1(run_touch_sensor_thread);
  while (1) {
    tud_task();  // tinyusb device task
    touch_stats_handler_task();
    hid_task();
    webserial_task();
    serial_console_task();
    led_blinking_task();
    teleplot_task();
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
  (void)remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
  blink_interval_ms = BLINK_MOUNTED;
}

//--------------------------------------------------------------------+
// WebUSB use vendor class
//--------------------------------------------------------------------+

// Invoked when a control transfer occurred on an interface of this class
// Driver response accordingly to the request and the transfer stage
// (setup/data/ack) return false to stall control endpoint (e.g unsupported
// request)
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request) {
  // nothing to with DATA & ACK stage
  if (stage != CONTROL_STAGE_SETUP)
    return true;

  switch (request->bmRequestType_bit.type) {
    case TUSB_REQ_TYPE_VENDOR:
      switch (request->bRequest) {
        case VENDOR_REQUEST_WEBUSB:
          // match vendor request in BOS descriptor
          // Get landing page url
          return tud_control_xfer(rhport, request, (void*)(uintptr_t)&desc_url, desc_url.bLength);

        case VENDOR_REQUEST_MICROSOFT:
          if (request->wIndex == 7) {
            // Get Microsoft OS 2.0 compatible descriptor
            uint16_t total_len;
            memcpy(&total_len, desc_ms_os_20 + 8, 2);

            return tud_control_xfer(rhport, request, (void*)(uintptr_t)desc_ms_os_20, total_len);
          } else {
            return false;
          }

        default:
          break;
      }
      break;

    case TUSB_REQ_TYPE_CLASS:
      if (request->bRequest == 0x22) {
        // Webserial simulate the CDC_REQUEST_SET_CONTROL_LINE_STATE (0x22) to
        // connect and disconnect.
        web_serial_connected = (request->wValue != 0);

        // Always lit LED if connected
        if (web_serial_connected) {
          board_led_write(true);
          blink_interval_ms = BLINK_ALWAYS_ON;

          tud_vendor_write_str("\r\nWebUSB interface connected\r\n");
          tud_vendor_flush();
        } else {
          blink_interval_ms = BLINK_MOUNTED;
        }

        // response with status OK
        return tud_control_status(rhport, request);
      }
      break;

    default:
      break;
  }

  // stall unknown request
  return false;
}

// send characters to both CDC and WebUSB
void echo_all(uint8_t buf[], uint32_t count) {
  // echo to web serial
  if (web_serial_connected) {
    tud_vendor_write(buf, count);
    tud_vendor_flush();
  }

  // echo to cdc
  if (tud_cdc_connected()) {
    for (uint32_t i = 0; i < count; i++) {
      tud_cdc_write_char(buf[i]);

      if (buf[i] == '\r')
        tud_cdc_write_char('\n');
    }
    tud_cdc_write_flush();
  }
}

void webserial_task(void) {
  if (web_serial_connected) {
    if (tud_vendor_available()) {
      uint8_t buf[64];
      uint32_t count = tud_vendor_read(buf, sizeof(buf));

      // echo back to both web serial and cdc
      echo_all(buf, count);
    }
  }
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+


#if 0
// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len)
{
    (void)instance;
    (void)len;

    uint8_t next_report_id = report[0] + 1;

    if (next_report_id < REPORT_ID_COUNT)
    {
        send_hid_report(next_report_id, board_button_read());
    }
}
#endif

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance,
                               uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t* buffer,
                               uint16_t reqlen) {
  // TODO not Implemented
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const* buffer,
                           uint16_t bufsize) {
  (void)instance;
  // do nothing
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void) {
  static uint32_t start_ms = 0;
  static bool led_state = false;

  if (!queue_is_empty(&q_blink_interval)) {
    queue_try_remove(&q_blink_interval, &blink_interval_ms);
  }
  if (blink_interval_ms == 0) {
    led_state = false;
    board_led_write(led_state);
    return;
  }

  // Blink every interval ms
  if (board_millis() - start_ms < blink_interval_ms)
    return;  // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state;  // toggle
}
