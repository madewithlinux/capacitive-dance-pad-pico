#include "tusb.h"

#include "custom_logging.hpp"

char __after_data("buffers") _global_format_buf[FORMAT_BUFFER_SIZE] = {0};

// int _cdc_puts(uint8_t itf, const char* str) {
//   tud_cdc_n_write_str(itf, str);
//   tud_cdc_n_write_char(itf, '\r');
//   tud_cdc_n_write_char(itf, '\n');
//   CDC_AUTO_FLUSH_IF_ENABLED(itf);
//   return 0;
// }

void _cdc_usb_out_chars(uint8_t itf, const char* buf, int length) {
  static uint64_t last_avail_time;
  // if (!mutex_try_enter_block_until(&stdio_usb_mutex,
  // make_timeout_time_ms(PICO_STDIO_DEADLOCK_TIMEOUT_MS))) {
  //     return;
  // }
  if (tud_cdc_n_connected(itf)) {
    for (int i = 0; i < length;) {
      int n = length - i;
      int avail = (int)tud_cdc_n_write_available(itf);
      if (n > avail)
        n = avail;
      if (n) {
        int n2 = (int)tud_cdc_n_write(itf, buf + i, (uint32_t)n);
        tud_task();
        tud_cdc_n_write_flush(itf);
        i += n2;
        last_avail_time = time_us_64();
      } else {
        tud_task();
        tud_cdc_n_write_flush(itf);
        if (!tud_cdc_n_connected(itf) ||
            (!tud_cdc_n_write_available(itf) &&
             time_us_64() >
                 last_avail_time + PICO_STDIO_USB_STDOUT_TIMEOUT_US)) {
          break;
        }
      }
    }
  } else {
    // reset our timeout
    last_avail_time = 0;
  }
  // mutex_exit(&stdio_usb_mutex);
}
