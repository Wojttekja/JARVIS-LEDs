// Integer HSV <-> RGB helpers (hue 0..359, saturation/value 0..255).
#pragma once

#include <stdint.h>
#include "protocol.h"

rgb_t hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v);
void rgb_to_hsv(rgb_t c, uint16_t *h, uint8_t *s, uint8_t *v);
