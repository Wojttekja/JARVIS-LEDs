#include "effects.h"

#include <math.h>
#include "color.h"
#include "config.h"
#include "pico/rand.h"

static inline float speed01(const led_state_t *s) {
    return s->speed / 255.0f;
}

static inline float rand01(void) {
    return (get_rand_32() & 0xFFFF) / 65535.0f;
}

static inline rgb_t scale(rgb_t c, float k) {
    return (rgb_t){(uint8_t)(c.r * k), (uint8_t)(c.g * k), (uint8_t)(c.b * k)};
}

// --- SOLID ----------------------------------------------------------------

static void solid(const led_state_t *s, rgb_t *out, uint16_t n) {
    for (uint16_t i = 0; i < n; i++) out[i] = s->color;
}

// --- RAINBOW --------------------------------------------------------------
// v0: one full rainbow spread around the strip, flowing. v1: whole strip one hue.

static void rainbow(const led_state_t *s, uint32_t t_ms, rgb_t *out, uint16_t n) {
    float deg_per_s = 10.0f + speed01(s) * 170.0f;
    float base = fmodf(t_ms / 1000.0f * deg_per_s, 360.0f);
    for (uint16_t i = 0; i < n; i++) {
        float hue = s->variant == 1 ? base * 0.5f : base + i * 360.0f / n;
        out[i] = hsv_to_rgb((uint16_t)hue % 360, 255, 255);
    }
}

// --- FIRE -----------------------------------------------------------------
// Candle-style: every pixel has a heat value chasing a random target, with
// occasional flares, slightly blurred with its neighbours, then mapped
// through a palette.

typedef struct {
    float pos;
    rgb_t c;
} stop_t;

static const stop_t PALETTES[FIRE_PALETTES][5] = {
    {{0.00f, {0, 0, 0}}, {0.35f, {160, 8, 0}},  {0.60f, {255, 70, 0}},  {0.85f, {255, 160, 30}}, {1.0f, {255, 230, 140}}},
    {{0.00f, {0, 0, 0}}, {0.35f, {0, 8, 150}},  {0.60f, {0, 70, 255}},  {0.85f, {50, 170, 255}}, {1.0f, {190, 235, 255}}},
    {{0.00f, {0, 0, 0}}, {0.35f, {8, 110, 0}},  {0.60f, {50, 255, 0}},  {0.85f, {170, 255, 30}}, {1.0f, {225, 255, 170}}},
};

static rgb_t gradient(const stop_t p[5], float x) {
    for (int i = 1; i < 5; i++) {
        if (x <= p[i].pos) {
            float k = (x - p[i - 1].pos) / (p[i].pos - p[i - 1].pos);
            rgb_t a = p[i - 1].c, b = p[i].c;
            return (rgb_t){(uint8_t)(a.r + (b.r - a.r) * k), (uint8_t)(a.g + (b.g - a.g) * k),
                           (uint8_t)(a.b + (b.b - a.b) * k)};
        }
    }
    return p[4].c;
}

static float heat[WS2811_MAX_PIXELS];
static float heat_target[WS2811_MAX_PIXELS];

static void fire(const led_state_t *s, uint32_t dt_ms, rgb_t *out, uint16_t n) {
    float dt = dt_ms / 1000.0f;
    float sp = speed01(s);
    float retarget_per_s = 3.0f + sp * 17.0f;  // how often each pixel picks a new level
    float follow = fminf(1.0f, dt * (4.0f + sp * 12.0f));

    for (uint16_t i = 0; i < n; i++) {
        if (rand01() < retarget_per_s * dt) {
            heat_target[i] = rand01() < 0.06f ? 1.0f : 0.40f + rand01() * 0.45f;  // flare / normal
        }
        heat[i] += (heat_target[i] - heat[i]) * follow;
    }
    for (uint16_t i = 0; i < n; i++) {
        float l = heat[(i + n - 1) % n], r = heat[(i + 1) % n];
        out[i] = gradient(PALETTES[s->variant % FIRE_PALETTES], 0.6f * heat[i] + 0.2f * (l + r));
    }
}

// --- BREATHE --------------------------------------------------------------

static void breathe(const led_state_t *s, uint32_t t_ms, rgb_t *out, uint16_t n) {
    float period_s = 8.0f - speed01(s) * 6.5f;  // 8 s .. 1.5 s
    float x = 0.5f - 0.5f * cosf(2.0f * (float)M_PI * (t_ms / 1000.0f) / period_s);
    float level = 0.04f + 0.96f * x * x;  // squared: looks more natural to the eye
    rgb_t c = scale(s->color, level);
    for (uint16_t i = 0; i < n; i++) out[i] = c;
}

// --- NEBULA ---------------------------------------------------------------
// Slowly drifting purple clouds: two waves travelling in opposite directions
// pick a shade from indigo to magenta, a third one varies the glow. Whole
// numbers of waves fit around the strip, so the two ends join seamlessly.

static const stop_t NEBULA[5] = {
    {0.00f, {25, 0, 70}}, {0.35f, {90, 0, 200}}, {0.65f, {170, 0, 255}}, {0.85f, {255, 20, 190}}, {1.0f, {255, 110, 230}},
};

static void nebula(const led_state_t *s, uint32_t t_ms, rgb_t *out, uint16_t n) {
    float t = t_ms / 1000.0f * (0.05f + speed01(s) * 0.6f);
    for (uint16_t i = 0; i < n; i++) {
        float x = 2.0f * (float)M_PI * i / n;
        float shade = 0.5f + 0.25f * (sinf(2.0f * x + 2.0f * t) + sinf(3.0f * x - 1.3f * t + 1.7f));
        float glow = 0.45f + 0.55f * (0.5f + 0.5f * sinf(x - 0.7f * t));
        out[i] = scale(gradient(NEBULA, shade), glow);
    }
}

void effects_render(const led_state_t *s, uint32_t t_ms, uint32_t dt_ms, rgb_t *out, uint16_t n) {
    switch (s->effect) {
        case EFFECT_RAINBOW: rainbow(s, t_ms, out, n); break;
        case EFFECT_FIRE:    fire(s, dt_ms, out, n); break;
        case EFFECT_BREATHE: breathe(s, t_ms, out, n); break;
        case EFFECT_NEBULA:  nebula(s, t_ms, out, n); break;
        default:             solid(s, out, n); break;
    }
}
