// Receiver settings: things tied to the physical strip. Everything you'd tweak
// day-to-day (colors, timer length, steps) lives in remote/config.h instead.
#pragma once

#include "protocol.h"
#include "ws2811.h"

// Strip (measured in step 7)
#define NUM_PIXELS        48
#define STRIP_ORDER       ORDER_BRG
#define STRIP_SLOW_400KHZ false

// Global cap applied on top of the remote's brightness (power budget / taste)
#define MAX_BRIGHTNESS 255

#define FRAME_MS        16   // ~60 fps
#define FADE_ON_OFF_MS  600  // on/off fade
#define CROSSFADE_MS    400  // effect / color change blend
#define SAVE_DELAY_MS   5000 // save state to flash once it's been stable this long
#define WATCHDOG_MS     1000

// State used on first boot (before anything was saved)
#define DEFAULT_STATE ((led_state_t){ \
    .on = 1, .effect = EFFECT_SOLID, .color = {255, 140, 40}, .brightness = 96, .speed = 128})
