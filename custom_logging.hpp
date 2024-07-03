#pragma once
#include "pico/stdlib.h"

#include "config_defines.h"

#define CDC_SERIAL0_ITF (0)
#define CDC_SERIAL1_ITF (1)

extern char __after_data("buffers") _global_format_buf[FORMAT_BUFFER_SIZE];

// adapted from pico-sdk stdio_usb.c
// writes the buffer to USB CDC.
// NOTE: does NOT use any mutex
void _cdc_usb_out_chars(uint8_t itf, const char* buf, int length);

// int _cdc_puts(uint8_t itf, const char* str);

#ifndef CDC_AUTO_FLUSH
#define CDC_AUTO_FLUSH 0
#endif

#if CDC_AUTO_FLUSH
#define CDC_AUTO_FLUSH_IF_ENABLED(itf) tud_cdc_n_write_flush(itf)
#else
#define CDC_AUTO_FLUSH_IF_ENABLED(itf)
#endif

inline int _cdc_puts(uint8_t itf, const char* str) {
  _cdc_usb_out_chars(itf, str, strlen(str));
  _cdc_usb_out_chars(itf, "\r\n", 2);
  // tud_cdc_n_write_str(itf, str);
  // tud_cdc_n_write_char(itf, '\r');
  // tud_cdc_n_write_char(itf, '\n');
  CDC_AUTO_FLUSH_IF_ENABLED(itf);
  return 0;
}
#define CDC_PUTS(itf, str) _cdc_puts(itf, str)

#define CDC_FLUSH(itf) tud_cdc_n_write_flush(itf)
#define CDC_IS_CONNECTED(itf) tud_cdc_n_connected(itf)

// TODO: handle encoding error case?
#define CDC_PRINTF(itf, fstr, ...)                                                               \
  do {                                                                                           \
    int len = snprintf(_global_format_buf, FORMAT_BUFFER_SIZE, fstr __VA_OPT__(, ) __VA_ARGS__); \
    if (len > 0) {                                                                               \
      _cdc_usb_out_chars(itf, _global_format_buf, MIN(len, FORMAT_BUFFER_SIZE));                 \
    }                                                                                            \
    CDC_AUTO_FLUSH_IF_ENABLED(itf);                                                              \
  } while (0)

#if SERIAL_TELEPLOT

// clang-format off
#define TELEPLOT_SERIAL_ITF           CDC_SERIAL1_ITF
#define teleplot_puts(str)            CDC_PUTS(TELEPLOT_SERIAL_ITF, str)
#define teleplot_flush()              CDC_FLUSH(TELEPLOT_SERIAL_ITF)
#define teleplot_is_connected()       CDC_IS_CONNECTED(TELEPLOT_SERIAL_ITF)
#define teleplot_printf(fstr, ...)    CDC_PRINTF(TELEPLOT_SERIAL_ITF, fstr __VA_OPT__(, ) __VA_ARGS__)
// clang-format on

#else
#endif
