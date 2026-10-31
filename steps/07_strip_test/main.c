// Step 7: WS2811 strip test on the desk (RECEIVER board), driven from serial.
//   o  color-order test: segment 1 RED, 2 GREEN, 3 BLUE. Press o again to try the next order.
//   + / -   move pixel-count marker by 1      > / <  by 10      } / {  by 100
//   w  all white          r  moving rainbow     x  all off
//   b  cycle brightness   s  toggle 800/400 kHz   h  help
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "board_id.h"
#include "color.h"
#include "ws2811.h"

static rgb_t pixels[WS2811_MAX_PIXELS];
static color_order_t order = ORDER_RGB;
static bool slow;
static uint8_t brightness = 64;
static int marker = 0;

typedef enum { MODE_OFF, MODE_ORDER, MODE_MARKER, MODE_WHITE, MODE_RAINBOW } test_mode_t;
static test_mode_t mode = MODE_OFF;

static rgb_t scale(rgb_t c) {
    return (rgb_t){c.r * brightness / 255, c.g * brightness / 255, c.b * brightness / 255};
}

static void help(void) {
    printf("\nCommands:\n"
           "  o      color-order test (segments 1-3 should be RED, GREEN, BLUE; o = next order)\n"
           "  + -    move count marker by 1   > <  by 10   } {  by 100\n"
           "  w      all white   r  rainbow   x  off\n"
           "  b      cycle brightness (now %u)   s  toggle 800/400 kHz (now %s)\n"
           "  h      this help\n\n",
           brightness, slow ? "400" : "800");
}

static void render(uint32_t frame) {
    memset(pixels, 0, sizeof(pixels));
    switch (mode) {
        case MODE_ORDER:
            pixels[0] = scale((rgb_t){255, 0, 0});
            pixels[1] = scale((rgb_t){0, 255, 0});
            pixels[2] = scale((rgb_t){0, 0, 255});
            break;
        case MODE_MARKER:
            for (int i = 0; i < marker; i++) {
                // dim blue trail, every 10th pixel dim green to help counting by eye
                pixels[i] = (i % 10 == 9) ? (rgb_t){0, 12, 0} : (rgb_t){0, 0, 8};
            }
            pixels[marker] = scale((rgb_t){255, 255, 255});
            break;
        case MODE_WHITE:
            for (int i = 0; i < WS2811_MAX_PIXELS; i++) pixels[i] = scale((rgb_t){255, 255, 255});
            break;
        case MODE_RAINBOW:
            for (int i = 0; i < WS2811_MAX_PIXELS; i++) {
                pixels[i] = hsv_to_rgb((i * 7 + frame * 2) % 360, 255, brightness);
            }
            break;
        default:
            break;
    }
    ws2811_show(pixels, WS2811_MAX_PIXELS);
}

static void move_marker(int delta) {
    marker += delta;
    if (marker < 0) marker = 0;
    if (marker >= WS2811_MAX_PIXELS) marker = WS2811_MAX_PIXELS - 1;
    mode = MODE_MARKER;
    printf("marker on pixel %d -> if this is the LAST lit segment, strip has %d pixels\n",
           marker, marker + 1);
}

int main(void) {
    stdio_init_all();
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    for (int i = 0; i < 50 && !stdio_usb_connected(); i++) {
        sleep_ms(100);
    }
    printf("\n=== pico-leds step 7: strip test on %s board ===\n", board_role_name(board_role()));
    printf("Data on GP%d, sending %d pixels\n", WS2811_PIN, WS2811_MAX_PIXELS);

    ws2811_init(order, slow);
    help();

    for (uint32_t frame = 0;; frame++) {
        int c = getchar_timeout_us(0);
        switch (c) {
            case 'o':
                if (mode == MODE_ORDER) order = (order + 1) % ORDER_COUNT;
                mode = MODE_ORDER;
                ws2811_set_order(order);
                printf("order %s: are segments 1,2,3 RED, GREEN, BLUE? if not press o again\n",
                       COLOR_ORDER_NAMES[order]);
                break;
            case '+': move_marker(1); break;
            case '-': move_marker(-1); break;
            case '>': move_marker(10); break;
            case '<': move_marker(-10); break;
            case '}': move_marker(100); break;
            case '{': move_marker(-100); break;
            case 'w': mode = MODE_WHITE; printf("all white\n"); break;
            case 'r': mode = MODE_RAINBOW; printf("rainbow\n"); break;
            case 'x': mode = MODE_OFF; printf("off\n"); break;
            case 'b':
                brightness = brightness >= 255 ? 16 : brightness * 2 > 255 ? 255 : brightness * 2;
                printf("brightness %u\n", brightness);
                break;
            case 's':
                slow = !slow;
                ws2811_set_slow(slow);
                printf("data rate %s kHz\n", slow ? "400" : "800");
                break;
            case 'h': help(); break;
            default: break;
        }

        render(frame);
        gpio_put(PICO_DEFAULT_LED_PIN, (frame / 30) % 2);  // heartbeat
        sleep_ms(16);  // ~60 fps
    }
}
