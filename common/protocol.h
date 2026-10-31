// Radio link parameters and message format shared by remote and receiver.
//
// The remote is the source of truth: every MSG_STATE carries the complete
// desired LED state, so a lost packet is fixed by the next one. The receiver
// answers every packet (inside the radio ACK) with a receiver_status_t, which
// lets the remote sync after a reboot and notice autonomous changes such as
// the sleep timer switching the lights off.
#pragma once

#include <stdint.h>

#define LINK_CHANNEL 108  // 2508 MHz, above WiFi channels 1-13
static const uint8_t LINK_ADDR[5] = {'L', 'E', 'D', 'S', '1'};

#define PROTOCOL_VERSION 1

typedef enum {
    EFFECT_SOLID = 0,
    EFFECT_RAINBOW,
    EFFECT_FIRE,
    EFFECT_BREATHE,
    EFFECT_NEBULA,
    EFFECT_COUNT,
} effect_t;

static const char *const EFFECT_NAMES[EFFECT_COUNT] = {
    "SOLID", "RAINBOW", "FIRE", "BREATHE", "NEBULA",
};

// Number of variants per effect (cycled by long-pressing the effect key)
#define RAINBOW_VARIANTS 2  // 0 = flowing along strip, 1 = whole strip same color
#define FIRE_PALETTES    3  // 0 = classic, 1 = blue gas flame, 2 = green toxic

typedef struct __attribute__((packed)) {
    uint8_t r, g, b;
} rgb_t;

typedef struct __attribute__((packed)) {
    uint8_t on;             // 0/1
    uint8_t effect;         // effect_t
    uint8_t variant;        // rainbow variant / fire palette
    rgb_t color;            // base color for SOLID, BREATHE
    uint8_t brightness;     // 1..255
    uint8_t speed;          // 1..255, effect speed
    uint8_t sleep_minutes;  // sleep timer length, 0 = none
    uint8_t sleep_id;       // incremented by the remote to (re)start the timer
} led_state_t;

typedef enum {
    MSG_STATE = 1,      // apply `state`
    MSG_GET_STATE = 2,  // heartbeat / sync request; `state` is ignored
} msg_type_t;

// Remote -> receiver
typedef struct __attribute__((packed)) {
    uint8_t version;  // PROTOCOL_VERSION
    uint8_t type;     // msg_type_t
    uint16_t seq;
    led_state_t state;
} remote_msg_t;

#define RECEIVER_FLAG_WATCHDOG_RESET 0x01  // last boot was a watchdog recovery

// Receiver -> remote (radio ACK payload; reflects the receiver *before* the
// packet it acknowledges, because it has to be queued in advance)
typedef struct __attribute__((packed)) {
    uint8_t version;
    uint8_t flags;             // RECEIVER_FLAG_*
    uint16_t last_state_seq;   // seq of the last MSG_STATE applied
    led_state_t state;         // what the receiver is showing
    uint16_t sleep_left_s;     // seconds until sleep timer turns off, 0 = none
    uint32_t uptime_s;
} receiver_status_t;

_Static_assert(sizeof(remote_msg_t) <= 32, "nRF24 payload limit");
_Static_assert(sizeof(receiver_status_t) <= 32, "nRF24 payload limit");

// Debug helper: prints one line describing the state, e.g.
// "<prefix> ON  FIRE    v1  color #FF8C28  bri  96  speed 128"
void print_led_state(const char *prefix, const led_state_t *s);
