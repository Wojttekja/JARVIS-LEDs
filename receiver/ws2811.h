// WS2811 strip driver: PIO generates the waveform, DMA feeds it.
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"

#define WS2811_PIN        2    // -> 74AHCT125 -> 330 R -> strip DIN
#define WS2811_MAX_PIXELS 600  // one pixel = one WS2811 chip (3 LEDs on 12 V strips)

// Order in which the strip expects the color bytes on the wire
typedef enum {
    ORDER_RGB, ORDER_RBG, ORDER_GRB, ORDER_GBR, ORDER_BRG, ORDER_BGR, ORDER_COUNT,
} color_order_t;

static const char *const COLOR_ORDER_NAMES[ORDER_COUNT] = {
    "RGB", "RBG", "GRB", "GBR", "BRG", "BGR",
};

void ws2811_init(color_order_t order, bool slow_400khz);
void ws2811_set_order(color_order_t order);
void ws2811_set_slow(bool slow_400khz);

// Starts sending `count` pixels (returns immediately; DMA does the work).
// Waits first if the previous frame is still going out / latching.
void ws2811_show(const rgb_t *pixels, uint16_t count);
