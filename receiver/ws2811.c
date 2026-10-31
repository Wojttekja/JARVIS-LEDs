#include "ws2811.h"

#include "hardware/dma.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "ws2811.pio.h"

#define RESET_US 300  // low time that latches the frame (WS2811 needs > 50 us)

static PIO pio = pio0;
static uint sm;
static uint offset;
static int dma_chan;
static color_order_t order;
static bool slow;
static uint32_t buffer[WS2811_MAX_PIXELS];
static absolute_time_t next_frame_allowed;

static void start_sm(void) {
    ws2811_program_init(pio, sm, offset, WS2811_PIN, slow ? 400000.0f : 800000.0f);
}

void ws2811_init(color_order_t initial_order, bool slow_400khz) {
    order = initial_order;
    slow = slow_400khz;

    sm = pio_claim_unused_sm(pio, true);
    offset = pio_add_program(pio, &ws2811_program);
    start_sm();

    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));
    dma_channel_configure(dma_chan, &c, &pio->txf[sm], buffer, 0, false);

    next_frame_allowed = get_absolute_time();
}

void ws2811_set_order(color_order_t new_order) {
    order = new_order;
}

void ws2811_set_slow(bool slow_400khz) {
    dma_channel_wait_for_finish_blocking(dma_chan);
    sleep_until(next_frame_allowed);
    slow = slow_400khz;
    pio_sm_set_enabled(pio, sm, false);
    start_sm();
}

static inline uint32_t pack(rgb_t c) {
    uint8_t a, b, d;
    switch (order) {
        case ORDER_RBG: a = c.r; b = c.b; d = c.g; break;
        case ORDER_GRB: a = c.g; b = c.r; d = c.b; break;
        case ORDER_GBR: a = c.g; b = c.b; d = c.r; break;
        case ORDER_BRG: a = c.b; b = c.r; d = c.g; break;
        case ORDER_BGR: a = c.b; b = c.g; d = c.r; break;
        default:        a = c.r; b = c.g; d = c.b; break;
    }
    return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)d << 8);  // PIO shifts out MSB first
}

void ws2811_show(const rgb_t *pixels, uint16_t count) {
    if (count > WS2811_MAX_PIXELS) count = WS2811_MAX_PIXELS;

    dma_channel_wait_for_finish_blocking(dma_chan);
    sleep_until(next_frame_allowed);  // previous frame fully shifted out + latched

    for (uint16_t i = 0; i < count; i++) buffer[i] = pack(pixels[i]);

    // DMA finishing != wire finished: 24 bits/pixel still go out after the last word
    uint32_t bit_ns = slow ? 2500 : 1250;
    uint32_t frame_us = (uint32_t)count * 24 * bit_ns / 1000;
    next_frame_allowed = make_timeout_time_us(frame_us + RESET_US);

    dma_channel_transfer_from_buffer_now(dma_chan, buffer, count);
}
