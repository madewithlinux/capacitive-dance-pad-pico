#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/timer.h"

#include "blink.pio.h"
#include "touch.pio.h"

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq)
{
    blink_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    printf("Blinking pin %d at %d Hz\n", pin, freq);

    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    pio->txf[sm] = (125000000 / (2 * freq)) - 3;
}

void read_touch_sensor(PIO pio, uint sm, uint offset, uint pin)
{
    gpio_disable_pulls(pin);
    gpio_set_drive_strength(pin, GPIO_DRIVE_STRENGTH_12MA);

    touch_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    int32_t touch_timeout = 1<<12;


    // sleep_ms(4000);
    // while (true)
    // {
    //     while (pio_sm_is_rx_fifo_empty(pio, sm))
    //     {
    //         tight_loop_contents();
    //     }
    //     int32_t value = touch_timeout - pio->rxf[sm];
    //     printf("capacitive read value: %u\n", value);
    //     sleep_ms(100);
    // }
    sleep_ms(2000);
    printf("begin reading PIO fifo\n");
    while (true)
    {
        int32_t value = touch_timeout - pio_sm_get_blocking(pio, sm);
        // int32_t value = pio_sm_get_blocking(pio, sm);
        printf("capacitive read value: %u\n", value);
        sleep_ms(250);
    }

}

int64_t alarm_callback(alarm_id_t id, void *user_data)
{
    // Put your timeout handler code in here
    return 0;
}

int main()
{
    stdio_init_all();

    sleep_ms(4000);

    // PIO Blinking example
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &touch_program);
    printf("Loaded program at %d\n", offset);

    read_touch_sensor(pio, 0, offset, 15);

// #ifdef PICO_DEFAULT_LED_PIN
//     blink_pin_forever(pio, 0, offset, PICO_DEFAULT_LED_PIN, 3);
// #else
//     blink_pin_forever(pio, 0, offset, 6, 3);
// #endif
    // For more pio examples see https://github.com/raspberrypi/pico-examples/tree/master/pio

    // Timer example code - This example fires off the callback after 2000ms
    add_alarm_in_ms(2000, alarm_callback, NULL, false);
    // For more examples of timer use see https://github.com/raspberrypi/pico-examples/tree/master/timer

    while (true)
    {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
