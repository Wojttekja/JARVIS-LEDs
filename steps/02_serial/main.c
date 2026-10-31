// Step 2: USB serial - print a counter + board ID, echo typed characters.
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/unique_id.h"

int main(void) {
    stdio_init_all();

    const uint led = PICO_DEFAULT_LED_PIN;
    gpio_init(led);
    gpio_set_dir(led, GPIO_OUT);

    // Wait (max 5 s) for the Mac to open the port so the banner isn't lost
    for (int i = 0; i < 50 && !stdio_usb_connected(); i++) {
        sleep_ms(100);
    }

    char id[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    pico_get_unique_board_id_string(id, sizeof(id));

    printf("\n=== pico-leds step 2: USB serial ===\n");
    printf("Board ID: %s\n", id);
    printf("Type something - it will be echoed back.\n\n");

    uint32_t counter = 0;
    absolute_time_t next_tick = get_absolute_time();

    while (true) {
        // Echo any typed character without blocking
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            printf("You typed: '%c' (0x%02x)\n", (c >= 32 && c < 127) ? c : '?', c);
        }

        // Once per second: print counter and toggle LED
        if (time_reached(next_tick)) {
            printf("tick %lu  (uptime %lu ms)\n", (unsigned long)counter++,
                   (unsigned long)to_ms_since_boot(get_absolute_time()));
            gpio_xor_mask(1u << led);
            next_tick = delayed_by_ms(next_tick, 1000);
        }
    }
}
