// Remote firmware: 3x3 keys + knob -> LED state -> nRF24 -> receiver.
//
//  K1 ON/OFF       (long: sleep timer)   K2 RAINBOW (long: next variant)   K3 FIRE (long: next palette)
//  K4 BREATHE                            K5 NEBULA                         K6 WARM WHITE
//  K7 color slot 1 (long: save current)  K8 slot 2 (long: save)            K9 slot 3 (long: save)
//  Knob: turn = adjust, press = cycle BRIGHTNESS -> SPEED -> HUE
//
// LED: short flash = state sent, N flashes = knob mode N / color saved,
//      continuous fast blink = receiver unreachable.
// Serial: 's' prints local state + receiver health, 'r' resets the link counters.
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "board_id.h"
#include "color.h"
#include "config.h"
#include "input.h"
#include "nrf24.h"
#include "protocol.h"
#include "storage.h"

enum { K_POWER, K_RAINBOW, K_FIRE, K_BREATHE, K_NEBULA, K_WARM, K_SLOT1, K_SLOT2, K_SLOT3 };

// Keys with a long-press action fire their short action on release instead of press
static const bool HAS_LONG[INPUT_NUM_BUTTONS] = {
    [K_POWER] = true, [K_RAINBOW] = true, [K_FIRE] = true,
    [K_SLOT1] = true, [K_SLOT2] = true,   [K_SLOT3] = true,
};

typedef enum { KNOB_BRIGHTNESS, KNOB_SPEED, KNOB_HUE, KNOB_MODE_COUNT } knob_mode_t;
static const char *const KNOB_MODE_NAMES[KNOB_MODE_COUNT] = {"BRIGHTNESS", "SPEED", "HUE"};

static led_state_t state = {
    .on = 1,
    .effect = EFFECT_SOLID,
    .color = COLOR_WARM_WHITE,
    .brightness = DEFAULT_BRIGHTNESS,
    .speed = DEFAULT_SPEED,
};
static uint8_t rainbow_variant, fire_palette;
static rgb_t slots[STORAGE_COLOR_SLOTS] = {COLOR_RED, COLOR_GREEN, COLOR_BLUE};

static bool dirty;   // state changed, not yet acknowledged by the receiver
static bool synced;  // state is known-good (synced from receiver or set by user)
static uint16_t seq, last_state_seq;
static bool link_ok = true;
static uint32_t sends_ok, sends_failed, receiver_restarts;
static receiver_status_t last_status;
static bool have_status;

static knob_mode_t knob_mode = KNOB_BRIGHTNESS;
static absolute_time_t knob_last_used;
static uint16_t knob_hue;
static uint8_t knob_sat, knob_val;

// --- Onboard LED patterns -------------------------------------------------

static const uint led = PICO_DEFAULT_LED_PIN;
static uint8_t blink_phases;
static uint16_t blink_ms;
static absolute_time_t blink_next;

static void led_blink(uint8_t count, uint16_t ms) {
    blink_phases = count * 2;
    blink_ms = ms;
    blink_next = get_absolute_time();
}

static void led_update(void) {
    if (blink_phases) {
        if (time_reached(blink_next)) {
            blink_phases--;
            gpio_put(led, blink_phases % 2);  // odd phases left = on
            blink_next = make_timeout_time_ms(blink_ms);
        }
        return;
    }
    gpio_put(led, !link_ok && (to_ms_since_boot(get_absolute_time()) / 100) % 2);
}

// --- State changes --------------------------------------------------------

static void changed(void) {
    dirty = true;
    synced = true;  // user intent wins over whatever the receiver had
}

static void set_effect(effect_t effect) {
    state.on = 1;
    state.effect = effect;
    state.variant = effect == EFFECT_RAINBOW ? rainbow_variant
                  : effect == EFFECT_FIRE    ? fire_palette
                                             : 0;
    changed();
}

// Color keys keep BREATHE (it uses the color) but leave effects with their own colors
static void set_color(rgb_t color) {
    state.on = 1;
    state.color = color;
    if (state.effect != EFFECT_SOLID && state.effect != EFFECT_BREATHE) {
        state.effect = EFFECT_SOLID;
        state.variant = 0;
    }
    changed();
}

static void short_press(uint8_t key) {
    switch (key) {
        case K_POWER:
            state.on = !state.on;
            state.sleep_minutes = 0;  // any manual on/off cancels the sleep timer
            changed();
            break;
        case K_RAINBOW: set_effect(EFFECT_RAINBOW); break;
        case K_FIRE:    set_effect(EFFECT_FIRE); break;
        case K_BREATHE: set_effect(EFFECT_BREATHE); break;
        case K_NEBULA:  set_effect(EFFECT_NEBULA); break;
        case K_WARM:    set_color(COLOR_WARM_WHITE); break;
        case K_SLOT1:
        case K_SLOT2:
        case K_SLOT3:   set_color(slots[key - K_SLOT1]); break;
    }
}

static void long_press(uint8_t key) {
    switch (key) {
        case K_POWER:
            state.on = 1;
            state.sleep_minutes = SLEEP_TIMER_MINUTES;
            state.sleep_id++;
            changed();
            printf("sleep timer: off in %d min\n", SLEEP_TIMER_MINUTES);
            led_blink(2, 150);
            break;
        case K_RAINBOW:
            rainbow_variant = (rainbow_variant + 1) % RAINBOW_VARIANTS;
            set_effect(EFFECT_RAINBOW);
            break;
        case K_FIRE:
            fire_palette = (fire_palette + 1) % FIRE_PALETTES;
            set_effect(EFFECT_FIRE);
            break;
        case K_SLOT1:
        case K_SLOT2:
        case K_SLOT3: {
            uint8_t slot = key - K_SLOT1;
            slots[slot] = state.color;
            bool ok = storage_save(slots);
            printf("slot %u = #%02X%02X%02X %s\n", slot + 1, state.color.r, state.color.g,
                   state.color.b, ok ? "saved" : "SAVE FAILED");
            led_blink(3, 80);
            break;
        }
    }
}

// --- Knob -----------------------------------------------------------------

static void knob_enter_hue_mode(void) {
    rgb_to_hsv(state.color, &knob_hue, &knob_sat, &knob_val);
    if (knob_sat < 40) {  // white-ish: rotating hue would do nothing visible
        knob_sat = 255;
        knob_val = 255;
    }
}

static void knob_turn(int8_t delta) {
    knob_last_used = get_absolute_time();
    switch (knob_mode) {
        case KNOB_BRIGHTNESS: {
            uint8_t b = state.brightness;
            const int n = sizeof(BRIGHTNESS_LEVELS);
            if (delta > 0) {
                for (int i = 0; i < n; i++) {
                    if (BRIGHTNESS_LEVELS[i] > b) { b = BRIGHTNESS_LEVELS[i]; break; }
                }
            } else {
                for (int i = n - 1; i >= 0; i--) {
                    if (BRIGHTNESS_LEVELS[i] < b) { b = BRIGHTNESS_LEVELS[i]; break; }
                }
            }
            state.brightness = b;
            state.on = 1;  // turning the knob on a dark strip should light it
            break;
        }
        case KNOB_SPEED: {
            int s = state.speed + delta * SPEED_STEP;
            state.speed = s < SPEED_STEP ? SPEED_STEP : s > 255 ? 255 : s;
            break;
        }
        case KNOB_HUE:
            knob_hue = (knob_hue + 360 + delta * HUE_STEP_DEG) % 360;
            set_color(hsv_to_rgb(knob_hue, knob_sat, knob_val));
            break;
        default:
            break;
    }
    changed();
    printf("knob %-10s -> bri %u  speed %u  color #%02X%02X%02X\n",
           KNOB_MODE_NAMES[knob_mode], state.brightness, state.speed, state.color.r,
           state.color.g, state.color.b);
}

static void knob_next_mode(void) {
    knob_mode = (knob_mode + 1) % KNOB_MODE_COUNT;
    knob_last_used = get_absolute_time();
    if (knob_mode == KNOB_HUE) knob_enter_hue_mode();
    printf("knob mode: %s\n", KNOB_MODE_NAMES[knob_mode]);
    led_blink(knob_mode + 1, 120);
}

static void knob_check_timeout(void) {
    if (knob_mode != KNOB_BRIGHTNESS &&
        absolute_time_diff_us(knob_last_used, get_absolute_time()) > KNOB_MODE_TIMEOUT_MS * 1000) {
        knob_mode = KNOB_BRIGHTNESS;
        printf("knob mode: BRIGHTNESS (timeout)\n");
    }
}

// --- Radio ----------------------------------------------------------------

static void handle_status(const receiver_status_t *st) {
    // Uptime going backwards = receiver rebooted (power cut, watchdog, reflash)
    if (have_status && st->uptime_s + 2 < last_status.uptime_s) {
        receiver_restarts++;
        printf("receiver RESTARTED after %lus up%s (restart #%lu)\n",
               (unsigned long)last_status.uptime_s,
               (st->flags & RECEIVER_FLAG_WATCHDOG_RESET) ? " - WATCHDOG recovery" : "",
               (unsigned long)receiver_restarts);
    }
    last_status = *st;
    have_status = true;
    if (dirty) return;  // our pending change wins

    // Adopt the receiver's state at boot, or when it changed on its own
    // (e.g. sleep timer) after applying everything we sent.
    if (!synced || st->last_state_seq == last_state_seq) {
        if (memcmp(&st->state, &state, sizeof(state)) != 0) {
            state = st->state;
            if (state.effect == EFFECT_RAINBOW) rainbow_variant = state.variant;
            if (state.effect == EFFECT_FIRE) fire_palette = state.variant;
            print_led_state(synced ? "receiver changed:" : "synced from receiver:", &state);
        }
        if (!synced) {
            last_state_seq = st->last_state_seq;
            if (seq < last_state_seq) seq = last_state_seq;
            synced = true;
        }
    }
}

static bool send_msg(msg_type_t type) {
    remote_msg_t msg = {
        .version = PROTOCOL_VERSION,
        .type = type,
        .seq = ++seq,
        .state = state,
    };
    bool ok = nrf24_send(&msg, sizeof(msg), NULL);

    while (nrf24_available()) {
        receiver_status_t st;
        if (nrf24_read(&st, sizeof(st)) == sizeof(st) && st.version == PROTOCOL_VERSION) {
            handle_status(&st);
        }
    }

    if (ok) sends_ok++; else sends_failed++;
    if (ok != link_ok) printf(ok ? "link: receiver reachable again\n" : "link: RECEIVER NOT RESPONDING\n");
    link_ok = ok;
    return ok;
}

static void print_status(void) {
    print_led_state("remote  :", &state);
    if (have_status) {
        print_led_state("receiver:", &last_status.state);
        printf("receiver: up %lus, last seq %u, sleep left %us | link %s, %s\n",
               (unsigned long)last_status.uptime_s, last_status.last_state_seq,
               last_status.sleep_left_s, link_ok ? "ok" : "DOWN", dirty ? "change pending" : "in sync");
        if (last_status.flags & RECEIVER_FLAG_WATCHDOG_RESET) {
            printf("receiver: !! its last boot was a WATCHDOG recovery (it hung once)\n");
        }
    } else {
        printf("receiver: no status received yet\n");
    }
    uint32_t total = sends_ok + sends_failed;
    printf("remote  : up %lus, sends %lu ok / %lu failed (%.2f%% loss), receiver restarts seen: %lu\n",
           (unsigned long)(to_ms_since_boot(get_absolute_time()) / 1000), (unsigned long)sends_ok,
           (unsigned long)sends_failed, total ? 100.0f * sends_failed / total : 0.0f,
           (unsigned long)receiver_restarts);
}

// --- Main -----------------------------------------------------------------

int main(void) {
    stdio_init_all();
    gpio_init(led);
    gpio_set_dir(led, GPIO_OUT);

    for (int i = 0; i < 30 && !stdio_usb_connected(); i++) {
        sleep_ms(100);
    }
    board_role_t role = board_role();
    printf("\n=== pico-leds REMOTE firmware (running on %s board) ===\n", board_role_name(role));
    if (role != BOARD_REMOTE) printf("WARNING: this is not the board labeled REMOTE\n");

    if (storage_load(slots)) {
        printf("loaded saved colors: #%02X%02X%02X #%02X%02X%02X #%02X%02X%02X\n",
               slots[0].r, slots[0].g, slots[0].b, slots[1].r, slots[1].g, slots[1].b,
               slots[2].r, slots[2].g, slots[2].b);
    }

    input_init();
    nrf24_init();
    nrf24_setup(LINK_CHANNEL, LINK_ADDR);

    absolute_time_t next_send = get_absolute_time();
    absolute_time_t next_heartbeat = get_absolute_time();
    bool long_fired[INPUT_NUM_BUTTONS] = {0};

    while (true) {
        input_event_t ev;
        while (input_poll(&ev)) {
            switch (ev.type) {
                case INPUT_PRESS:
                    long_fired[ev.key] = false;
                    if (ev.key == INPUT_KEY_ENC) {
                        knob_next_mode();
                    } else if (!HAS_LONG[ev.key]) {
                        short_press(ev.key);
                    }
                    break;
                case INPUT_LONG_PRESS:
                    if (HAS_LONG[ev.key]) {
                        long_fired[ev.key] = true;
                        long_press(ev.key);
                    }
                    break;
                case INPUT_RELEASE:
                    if (HAS_LONG[ev.key] && !long_fired[ev.key]) short_press(ev.key);
                    break;
                case INPUT_TURN:
                    knob_turn(ev.delta);
                    break;
            }
        }
        knob_check_timeout();

        int c = getchar_timeout_us(0);
        if (c == 's') print_status();
        if (c == 'r') {
            sends_ok = sends_failed = receiver_restarts = 0;
            printf("counters reset\n");
        }

        if (dirty) {
            if (time_reached(next_send)) {
                if (send_msg(MSG_STATE)) {
                    dirty = false;
                    last_state_seq = seq;
                    print_led_state("sent:", &state);
                    led_blink(1, 30);
                    next_send = make_timeout_time_ms(SEND_MIN_INTERVAL_MS);
                } else {
                    next_send = make_timeout_time_ms(RETRY_INTERVAL_MS);
                }
                next_heartbeat = make_timeout_time_ms(HEARTBEAT_MS);
            }
        } else if (time_reached(next_heartbeat)) {
            send_msg(MSG_GET_STATE);
            // Poll fast until we've synced once, then relax
            next_heartbeat = make_timeout_time_ms(synced && link_ok ? HEARTBEAT_MS : 250);
        }

        led_update();
        sleep_ms(1);
    }
}
