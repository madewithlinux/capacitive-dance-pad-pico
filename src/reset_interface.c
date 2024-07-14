#include "tusb.h"

#include "pico/bootrom.h"
#include "pico/stdio_usb.h"

#define PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED 25

#if PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE
// Support for default BOOTSEL reset by changing baud rate
void tud_cdc_line_coding_cb(__unused uint8_t itf, cdc_line_coding_t const* p_line_coding) {
    if (p_line_coding->bit_rate == PICO_STDIO_USB_RESET_MAGIC_BAUD_RATE) {
#ifdef PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED
        const uint gpio_mask = 1u << PICO_STDIO_USB_RESET_BOOTSEL_ACTIVITY_LED;
#else
        const uint gpio_mask = 0u;
#endif
        reset_usb_boot(gpio_mask, PICO_STDIO_USB_RESET_BOOTSEL_INTERFACE_DISABLE_MASK);
    }
}
#endif

