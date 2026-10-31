// Receiver firmware: nRF24 -> LED state -> effects -> WS2811 strip.
// Keeps a fresh receiver_status_t queued so every radio ACK carries it.
// Onboard LED toggles on every received packet. Serial: 's' prints status,
// 'k' knocks out the radio setup to test the self-repair.
#include <stdio.h>
#include <string.h>
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "board_id.h"
#include "config.h"
#include "effects.h"
#include "flash_store.h"
#include "nrf24.h"
#include "protocol.h"
#include "ws2811.h"

#define STATE_MAGIC 0x52435653u  // "RCVS"

static led_state_t state;
static uint16_t last_state_seq;
static uint32_t packets, heartbeats;
static uint8_t status_flags;
static uint32_t radio_reinits;
static absolute_time_t last_packet_at, next_radio_check;

// Sleep timer
static uint8_t seen_sleep_id;
static bool sleep_active;
static absolute_time_t sleep_deadline;
static uint32_t sleep_total_ms;

// Persistence
static led_state_t saved_state;
static bool save_pending;
static absolute_time_t save_at;

// Rendering
static rgb_t frame[NUM_PIXELS];     // current effect, full brightness
static rgb_t snapshot[NUM_PIXELS];  // previous effect, frozen at the moment of change
static rgb_t out[NUM_PIXELS];
static bool crossfading;
static absolute_time_t crossfade_start;
static float shown_brightness;      // eases towards state.brightness
static float master;                // 0..1 on/off fade

// --- State ----------------------------------------------------------------

// What gets saved: no running sleep timer (after a power cut it's simply off)
static led_state_t persistent(const led_state_t *s) {
    led_state_t p = *s;
    p.sleep_minutes = 0;
    return p;
}

static void schedule_save(void) {
    save_pending = true;
    save_at = make_timeout_time_ms(SAVE_DELAY_MS);
}

static void check_save(void) {
    if (!save_pending || !time_reached(save_at)) return;
    save_pending = false;
    led_state_t p = persistent(&state);
    if (memcmp(&p, &saved_state, sizeof(p)) == 0) return;
    if (flash_store_save(STATE_MAGIC, &p, sizeof(p))) {
        saved_state = p;
        printf("state saved to flash\n");
    } else {
        printf("FLASH SAVE FAILED\n");
    }
}

static void apply_state(const led_state_t *next) {
    bool looks_different = next->effect != state.effect || next->variant != state.variant ||
                           memcmp(&next->color, &state.color, sizeof(rgb_t)) != 0;
    if (looks_different && master > 0.0f) {
        memcpy(snapshot, frame, sizeof(frame));
        crossfading = true;
        crossfade_start = get_absolute_time();
    }

    // Sleep timer: a new sleep_id starts it; minutes == 0 or switching off cancels it.
    // Comparing against the last *seen* id means a stale packet can't restart it.
    if (next->sleep_minutes && next->sleep_id != seen_sleep_id) {
        sleep_active = true;
        sleep_total_ms = (uint32_t)next->sleep_minutes * 60 * 1000;
        sleep_deadline = make_timeout_time_ms(sleep_total_ms);
        printf("sleep timer started: %u min\n", next->sleep_minutes);
    }
    if (!next->sleep_minutes || !next->on) sleep_active = false;
    seen_sleep_id = next->sleep_id;

    state = *next;
    if (!sleep_active) state.sleep_minutes = 0;
    schedule_save();
}

static void check_sleep_timer(void) {
    if (sleep_active && time_reached(sleep_deadline)) {
        sleep_active = false;
        state.on = 0;
        state.sleep_minutes = 0;
        schedule_save();
        print_led_state("sleep timer done:", &state);
    }
}

// --- Radio ----------------------------------------------------------------

static void queue_status(void) {
    receiver_status_t st = {
        .version = PROTOCOL_VERSION,
        .flags = status_flags,
        .last_state_seq = last_state_seq,
        .state = state,
        .uptime_s = to_ms_since_boot(get_absolute_time()) / 1000,
    };
    if (sleep_active) {
        int64_t left_us = absolute_time_diff_us(get_absolute_time(), sleep_deadline);
        st.sleep_left_s = left_us > 0 ? left_us / 1000000 : 0;
    }
    nrf24_queue_ack_payload(&st, sizeof(st));  // no-op while one is already queued
}

static void poll_radio(void) {
    queue_status();
    if (!nrf24_available()) return;

    remote_msg_t msg;
    uint8_t len = nrf24_read(&msg, sizeof(msg));
    packets++;
    gpio_xor_mask(1u << PICO_DEFAULT_LED_PIN);

    if (len != sizeof(msg) || msg.version != PROTOCOL_VERSION) {
        printf("ignored packet (len %u, version %u)\n", len, len ? msg.version : 0);
    } else if (msg.type == MSG_STATE) {
        last_packet_at = get_absolute_time();
        apply_state(&msg.state);
        last_state_seq = msg.seq;
        char prefix[16];
        snprintf(prefix, sizeof(prefix), "#%-5u", msg.seq);
        print_led_state(prefix, &state);
    } else if (msg.type == MSG_GET_STATE) {
        last_packet_at = get_absolute_time();
        heartbeats++;
    }
}

static void radio_start(void) {
    nrf24_setup(LINK_CHANNEL, LINK_ADDR);
    nrf24_start_listening();
    last_packet_at = get_absolute_time();
}

// The radio can lose its setup on a supply glitch while the Pico keeps running
// (the watchdog can't see that), so check it and set it up again if needed.
static void check_radio(void) {
    if (!time_reached(next_radio_check)) return;
    next_radio_check = make_timeout_time_ms(RADIO_CHECK_MS);

    const char *why;
    if (!nrf24_config_intact(LINK_CHANNEL)) {
        why = "registers lost";
        status_flags |= RECEIVER_FLAG_RADIO_REINIT;
    } else if (absolute_time_diff_us(last_packet_at, get_absolute_time()) > RADIO_SILENCE_MS * 1000ll) {
        why = "no packets for a while";  // also happens simply when the remote is unplugged
    } else {
        return;
    }
    radio_reinits++;
    printf("radio set up again (%s), #%lu\n", why, (unsigned long)radio_reinits);
    radio_start();
}

// --- Rendering ------------------------------------------------------------

static void render(uint32_t now_ms, uint32_t dt_ms) {
    effects_render(&state, now_ms, dt_ms, frame, NUM_PIXELS);

    // Crossfade from the frozen previous effect
    float mix = 1.0f;
    if (crossfading) {
        mix = absolute_time_diff_us(crossfade_start, get_absolute_time()) / 1000.0f / CROSSFADE_MS;
        if (mix >= 1.0f) {
            mix = 1.0f;
            crossfading = false;
        }
    }

    // On/off fade and smoothed brightness
    float step = (float)dt_ms / FADE_ON_OFF_MS;
    master = state.on ? (master + step > 1.0f ? 1.0f : master + step)
                      : (master - step < 0.0f ? 0.0f : master - step);
    float ease = dt_ms * 0.012f;
    shown_brightness += (state.brightness - shown_brightness) * (ease > 1.0f ? 1.0f : ease);

    // Sleep timer: dim gradually over the whole duration
    float sleep_factor = 1.0f;
    if (sleep_active) {
        float left = absolute_time_diff_us(get_absolute_time(), sleep_deadline) / 1000.0f;
        float f = left > 0 ? left / sleep_total_ms : 0.0f;
        sleep_factor = f * f;
    }

    float k = (shown_brightness / 255.0f) * (master * master) * sleep_factor * (MAX_BRIGHTNESS / 255.0f);
    for (int i = 0; i < NUM_PIXELS; i++) {
        rgb_t a = snapshot[i], b = frame[i];
        out[i] = (rgb_t){
            (uint8_t)((a.r + (b.r - a.r) * mix) * k),
            (uint8_t)((a.g + (b.g - a.g) * mix) * k),
            (uint8_t)((a.b + (b.b - a.b) * mix) * k),
        };
    }
    ws2811_show(out, NUM_PIXELS);
}

static void print_status(void) {
    print_led_state("state:", &state);
    printf("uptime %lus, packets %lu (heartbeats %lu), last seq %u, radio re-setups %lu%s, sleep %s",
           (unsigned long)(to_ms_since_boot(get_absolute_time()) / 1000), (unsigned long)packets,
           (unsigned long)heartbeats, last_state_seq, (unsigned long)radio_reinits,
           (status_flags & RECEIVER_FLAG_RADIO_REINIT) ? " (registers were lost!)" : "",
           sleep_active ? "running" : "off");
    if (sleep_active) {
        printf(" (%lld s left)", absolute_time_diff_us(get_absolute_time(), sleep_deadline) / 1000000);
    }
    printf("\n");
}

// --- Main -----------------------------------------------------------------

int main(void) {
    stdio_init_all();  // no waiting for USB: on the ceiling nobody's listening
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    bool watchdog_reset = watchdog_enable_caused_reboot();  // real timeouts only, not picotool reboots
    if (watchdog_reset) status_flags |= RECEIVER_FLAG_WATCHDOG_RESET;

    if (!flash_store_load(STATE_MAGIC, &state, sizeof(state))) state = DEFAULT_STATE;
    saved_state = persistent(&state);
    seen_sleep_id = state.sleep_id;
    shown_brightness = state.brightness;
    master = 0.0f;  // fade in from black

    ws2811_init(STRIP_ORDER, STRIP_SLOW_400KHZ);
    nrf24_init();
    radio_start();

    watchdog_enable(WATCHDOG_MS, true);

    printf("\n=== pico-leds RECEIVER firmware (running on %s board)%s ===\n",
           board_role_name(board_role()), watchdog_reset ? " - RECOVERED FROM WATCHDOG RESET" : "");

    absolute_time_t next_frame = get_absolute_time();
    uint32_t last_frame_ms = to_ms_since_boot(next_frame);

    while (true) {
        watchdog_update();
        poll_radio();
        check_radio();
        check_sleep_timer();
        check_save();

        int c = getchar_timeout_us(0);
        if (c == 's') print_status();
        if (c == 'k') nrf24_write_reg(NRF24_REG_CONFIG, 0);  // simulate a radio glitch (powers it down)

        if (time_reached(next_frame)) {
            next_frame = delayed_by_ms(next_frame, FRAME_MS);
            uint32_t now_ms = to_ms_since_boot(get_absolute_time());
            render(now_ms, now_ms - last_frame_ms);
            last_frame_ms = now_ms;
        }
    }
}
