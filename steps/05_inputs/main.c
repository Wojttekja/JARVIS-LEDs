// Step 5: remote inputs - log every key event and encoder detent.
// LED lights while any key is held.
#include <stdio.h>
#include "pico/stdlib.h"
#include "board_id.h"
#include "input.h"

static const char *key_name(uint8_t key) {
    static const char *names[INPUT_NUM_BUTTONS] = {
        "K1", "K2", "K3", "K4", "K5", "K6", "K7", "K8", "K9", "ENC-BUTTON",
    };
    return key < INPUT_NUM_BUTTONS ? names[key] : "?";
}

int main(void) {
    stdio_init_all();
    const uint led = PICO_DEFAULT_LED_PIN;
    gpio_init(led);
    gpio_set_dir(led, GPIO_OUT);

    for (int i = 0; i < 50 && !stdio_usb_connected(); i++) {
        sleep_ms(100);
    }
    printf("\n=== pico-leds step 5: inputs on %s board ===\n", board_role_name(board_role()));
    printf("Keys K1-K9 = GP2-GP10, encoder CLK/DT/SW = GP11/GP12/GP13\n");
    printf("Press keys and turn the knob...\n\n");

    input_init();

    int32_t position = 0;
    int held = 0;
    input_event_t ev;
    char trace[128] = "";
    size_t trace_len = 0;
    absolute_time_t last_trace = get_absolute_time();
    uint8_t raw;

    while (true) {
        while (input_poll(&ev)) {
            uint32_t now = to_ms_since_boot(get_absolute_time());
            switch (ev.type) {
                case INPUT_PRESS:
                    held++;
                    printf("%7lu ms  %-10s press\n", (unsigned long)now, key_name(ev.key));
                    break;
                case INPUT_RELEASE:
                    held--;
                    printf("%7lu ms  %-10s release\n", (unsigned long)now, key_name(ev.key));
                    break;
                case INPUT_LONG_PRESS:
                    printf("%7lu ms  %-10s LONG press\n", (unsigned long)now, key_name(ev.key));
                    break;
                case INPUT_TURN:
                    position += ev.delta;
                    // drain raw states up to now so the step shows its own sequence
                    while (input_encoder_trace_pop(&raw) && trace_len < sizeof(trace) - 4) {
                        trace_len += snprintf(trace + trace_len, sizeof(trace) - trace_len,
                                              "%s%d%d", trace_len ? ">" : "", (raw >> 1) & 1,
                                              raw & 1);
                    }
                    printf("%7lu ms  knob %s  position %ld   raw DT,CLK: %s\n",
                           (unsigned long)now, ev.delta > 0 ? "CW  +1" : "CCW -1",
                           (long)position, trace);
                    trace_len = 0;
                    trace[0] = 0;
                    break;
            }
        }
        // Encoder diagnostics: collect raw pin states (as "DT CLK" bits) and
        // print them; anything left over without a step means a missed click.
        while (input_encoder_trace_pop(&raw) && trace_len < sizeof(trace) - 4) {
            trace_len += snprintf(trace + trace_len, sizeof(trace) - trace_len, "%s%d%d",
                                  trace_len ? ">" : "", (raw >> 1) & 1, raw & 1);
            last_trace = get_absolute_time();
        }
        if (trace_len && absolute_time_diff_us(last_trace, get_absolute_time()) > 400000) {
            printf("           raw DT,CLK: %s  (no step!)\n", trace);
            trace_len = 0;
            trace[0] = 0;
        }

        gpio_put(led, held > 0);
        sleep_ms(1);
    }
}
