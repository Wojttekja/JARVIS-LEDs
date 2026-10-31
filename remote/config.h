// Remote tunables - everything you might want to tweak lives here.
// Change, rebuild, reflash the remote; the receiver on the ceiling never needs touching.
#pragma once

#include "protocol.h"

// Preset colors (R, G, B). K7-K9 can be overwritten at runtime by a long press
// (saved in flash); these are the factory defaults.
#define COLOR_WARM_WHITE ((rgb_t){255, 140, 40})
#define COLOR_RED        ((rgb_t){255, 0, 0})
#define COLOR_GREEN      ((rgb_t){0, 255, 0})
#define COLOR_BLUE       ((rgb_t){0, 0, 255})

// State used when the receiver can't be reached at boot
#define DEFAULT_BRIGHTNESS 96
#define DEFAULT_SPEED      128

#define SLEEP_TIMER_MINUTES 30

// Knob
#define HUE_STEP_DEG         15    // per click in HUE mode (24 clicks = full color wheel)
#define SPEED_STEP           17    // per click in SPEED mode (15 clicks = full range)
#define KNOB_MODE_TIMEOUT_MS 8000  // fall back to BRIGHTNESS after this long idle

// Brightness steps, roughly perceptually even (one knob click each; ~1 turn min->max)
static const uint8_t BRIGHTNESS_LEVELS[] = {
    2, 4, 6, 9, 13, 18, 24, 31, 40, 50, 62, 76, 92, 110, 130, 153, 178, 205, 235, 255,
};

// Radio timing
#define SEND_MIN_INTERVAL_MS 20    // rate limit while spinning the knob
#define RETRY_INTERVAL_MS    200   // after a failed send
#define HEARTBEAT_MS         2000  // status poll when idle
