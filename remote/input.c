#include "input.h"

#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/util/queue.h"

#define DEBOUNCE_MS 5  // state must be stable this long to count

// Flip if turning clockwise gives -1
#define ENCODER_REVERSE 1

static queue_t events;
static repeating_timer_t scan_timer;

// --- Keys -----------------------------------------------------------------

typedef struct {
    uint8_t pin;
    bool pressed;       // debounced state
    uint8_t stable_ms;  // how long the raw reading has disagreed with `pressed`
    uint32_t held_ms;
    bool long_sent;
} button_t;

static button_t buttons[INPUT_NUM_BUTTONS];

static void push(input_event_type_t type, uint8_t key, int8_t delta) {
    input_event_t ev = {.type = type, .key = key, .delta = delta};
    queue_try_add(&events, &ev);  // drop if full; main loop is too slow anyway
}

static void scan_encoder(void);

static bool scan_inputs(repeating_timer_t *t) {
    (void)t;
    for (uint8_t i = 0; i < INPUT_NUM_BUTTONS; i++) {
        button_t *b = &buttons[i];
        bool raw = !gpio_get(b->pin);  // active low

        if (raw != b->pressed) {
            if (++b->stable_ms >= DEBOUNCE_MS) {
                b->pressed = raw;
                b->stable_ms = 0;
                b->held_ms = 0;
                b->long_sent = false;
                push(raw ? INPUT_PRESS : INPUT_RELEASE, i, 0);
            }
        } else {
            b->stable_ms = 0;
        }

        if (b->pressed && !b->long_sent && ++b->held_ms >= INPUT_LONG_PRESS_MS) {
            b->long_sent = true;
            push(INPUT_LONG_PRESS, i, 0);
        }
    }
    scan_encoder();
    return true;  // keep repeating
}

// --- Encoder --------------------------------------------------------------
// This KY-040 is a half-step type: it rests at CLK=DT=0 *and* CLK=DT=1, and each
// click moves CLK and DT once. Both pins are debounced in the 1 ms scan; every
// settled CLK change is one click. Direction = which pin moved first:
// DT already equals the new CLK -> DT led -> clockwise.

#define ENC_DEBOUNCE_MS 2

typedef struct {
    bool level;      // debounced
    uint8_t count;   // ms the raw reading has disagreed with `level`
} pin_filter_t;

static pin_filter_t clk_f, dt_f;
static queue_t trace;          // raw (DT<<1 | CLK) states, for diagnostics
static uint8_t last_raw = 0xFF;

// Returns true when the debounced level changes
static bool filter(pin_filter_t *f, bool raw) {
    if (raw == f->level) {
        f->count = 0;
        return false;
    }
    if (++f->count < ENC_DEBOUNCE_MS) return false;
    f->level = raw;
    f->count = 0;
    return true;
}

static void scan_encoder(void) {
    bool clk = gpio_get(INPUT_ENC_CLK_PIN);
    bool dt = gpio_get(INPUT_ENC_DT_PIN);

    uint8_t raw = (dt << 1) | clk;
    if (raw != last_raw) {
        last_raw = raw;
        queue_try_add(&trace, &raw);
    }

    filter(&dt_f, dt);  // DT first: a same-tick change counts as "DT led"
    if (filter(&clk_f, clk)) {
        int8_t dir = (clk_f.level == dt_f.level) ? 1 : -1;
        push(INPUT_TURN, 0, ENCODER_REVERSE ? -dir : dir);
    }
}

// --- Public ---------------------------------------------------------------

void input_init(void) {
    queue_init(&events, sizeof(input_event_t), 32);

    for (uint8_t i = 0; i < INPUT_NUM_BUTTONS; i++) {
        uint8_t pin = i < INPUT_NUM_KEYS ? INPUT_KEY_FIRST_PIN + i : INPUT_ENC_SW_PIN;
        buttons[i] = (button_t){.pin = pin};
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

    // KY-040 has its own 10k pull-ups on CLK/DT; internal ones don't hurt
    const uint enc_pins[] = {INPUT_ENC_CLK_PIN, INPUT_ENC_DT_PIN};
    for (int i = 0; i < 2; i++) {
        gpio_init(enc_pins[i]);
        gpio_set_dir(enc_pins[i], GPIO_IN);
        gpio_pull_up(enc_pins[i]);
    }
    queue_init(&trace, sizeof(uint8_t), 64);

    sleep_ms(10);  // let pull-ups settle before the first scan
    clk_f.level = gpio_get(INPUT_ENC_CLK_PIN);
    dt_f.level = gpio_get(INPUT_ENC_DT_PIN);
    add_repeating_timer_ms(-1, scan_inputs, NULL, &scan_timer);
}

bool input_poll(input_event_t *ev) {
    return queue_try_remove(&events, ev);
}

bool input_encoder_trace_pop(uint8_t *raw_state) {
    return queue_try_remove(&trace, raw_state);
}
