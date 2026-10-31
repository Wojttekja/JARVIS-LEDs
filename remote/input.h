// Remote inputs: up to 9 mechanical keys + KY-040 rotary encoder.
// Keys and encoder are scanned and debounced in a 1 ms timer.
// Events are queued and read with input_poll() from the main loop.
#pragma once

#include <stdbool.h>
#include <stdint.h>

// Keys: GP2..GP10 to GND (internal pull-ups)
#define INPUT_KEY_FIRST_PIN 2
#define INPUT_NUM_KEYS      9

// KY-040 (power its + pin from 3.3 V!)
#define INPUT_ENC_CLK_PIN 11
#define INPUT_ENC_DT_PIN  12
#define INPUT_ENC_SW_PIN  13

// Key index of the encoder push button (after the 9 keys)
#define INPUT_KEY_ENC  INPUT_NUM_KEYS
#define INPUT_NUM_BUTTONS (INPUT_NUM_KEYS + 1)

#define INPUT_LONG_PRESS_MS 600

typedef enum {
    INPUT_PRESS,       // key went down
    INPUT_RELEASE,     // key went up
    INPUT_LONG_PRESS,  // key held for INPUT_LONG_PRESS_MS (fires once, before release)
    INPUT_TURN,        // encoder moved one detent; delta = +1 (CW) / -1 (CCW)
} input_event_type_t;

typedef struct {
    input_event_type_t type;
    uint8_t key;   // 0..8 = keys, INPUT_KEY_ENC = encoder button
    int8_t delta;  // INPUT_TURN only
} input_event_t;

void input_init(void);

// Pops the next event. Returns false if the queue is empty.
bool input_poll(input_event_t *ev);

// Diagnostics: pops the next raw encoder pin state (DT<<1 | CLK), recorded on change.
bool input_encoder_trace_pop(uint8_t *raw_state);
