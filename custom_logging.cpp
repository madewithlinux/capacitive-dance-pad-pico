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
