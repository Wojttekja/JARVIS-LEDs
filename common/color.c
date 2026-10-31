#include "color.h"

rgb_t hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v) {
    h %= 360;
    uint8_t region = h / 60;
    uint32_t rem = (h % 60) * 255 / 60;
    uint8_t p = v * (255 - s) / 255;
    uint8_t q = v * (255 - s * rem / 255) / 255;
    uint8_t t = v * (255 - s * (255 - rem) / 255) / 255;

    switch (region) {
        case 0:  return (rgb_t){v, t, p};
        case 1:  return (rgb_t){q, v, p};
        case 2:  return (rgb_t){p, v, t};
        case 3:  return (rgb_t){p, q, v};
        case 4:  return (rgb_t){t, p, v};
        default: return (rgb_t){v, p, q};
    }
}

void rgb_to_hsv(rgb_t c, uint16_t *h, uint8_t *s, uint8_t *v) {
    uint8_t max = c.r > c.g ? (c.r > c.b ? c.r : c.b) : (c.g > c.b ? c.g : c.b);
    uint8_t min = c.r < c.g ? (c.r < c.b ? c.r : c.b) : (c.g < c.b ? c.g : c.b);
    int32_t delta = max - min;

    *v = max;
    *s = max ? delta * 255 / max : 0;
    if (delta == 0) {
        *h = 0;
        return;
    }

    int32_t hue;
    if (max == c.r) {
        hue = 60 * (c.g - c.b) / delta;
    } else if (max == c.g) {
        hue = 120 + 60 * (c.b - c.r) / delta;
    } else {
        hue = 240 + 60 * (c.r - c.g) / delta;
    }
    if (hue < 0) hue += 360;
    *h = hue;
}
