// Effect renderers. They draw at full brightness; main.c applies brightness,
// on/off fades, crossfades and the sleep-timer dimming on top.
#pragma once

#include <stdint.h>
#include "protocol.h"

// Renders one frame of s->effect into out[0..n-1].
// t_ms = time since boot, dt_ms = time since the previous frame.
void effects_render(const led_state_t *s, uint32_t t_ms, uint32_t dt_ms, rgb_t *out, uint16_t n);
