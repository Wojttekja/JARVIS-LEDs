#include "protocol.h"

#include <stdio.h>

void print_led_state(const char *prefix, const led_state_t *s) {
    const char *effect = s->effect < EFFECT_COUNT ? EFFECT_NAMES[s->effect] : "?";
    printf("%s %-3s %-7s v%u  color #%02X%02X%02X  bri %3u  speed %3u", prefix,
           s->on ? "ON" : "OFF", effect, s->variant, s->color.r, s->color.g, s->color.b,
           s->brightness, s->speed);
    if (s->sleep_minutes) printf("  sleep %um (id %u)", s->sleep_minutes, s->sleep_id);
    printf("\n");
}
