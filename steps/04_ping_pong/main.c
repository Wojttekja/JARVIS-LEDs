// Step 4: radio link test.
// REMOTE board sends 10 packets/s; RECEIVER board answers inside the ACK.
// Both print link statistics once per second. Role comes from the board ID.
//
// REMOTE serial keys: 0-3 = PA level (-18/-12/-6/0 dBm), r = reset totals.
// LED: REMOTE flashes on each ACKed packet, RECEIVER toggles on each packet.
#include <stdio.h>
#include "pico/stdlib.h"
#include "board_id.h"
#include "nrf24.h"
#include "protocol.h"

typedef struct {
    uint32_t seq;
    uint32_t tx_ms;
} ping_t;

typedef struct {
    uint32_t rx_count;  // packets the receiver has seen (as of its previous ACK)
} pong_t;

static const uint led = PICO_DEFAULT_LED_PIN;

static void run_remote(void) {
    static const char *pa_names[] = {"-18", "-12", "-6", "0"};
    uint8_t pa = 3;
    uint32_t seq = 0;
    uint32_t sec_sent = 0, sec_ok = 0, sec_retries = 0;
    uint32_t total_sent = 0, total_ok = 0;
    uint32_t last_pong = 0;
    absolute_time_t next_send = get_absolute_time();
    absolute_time_t next_report = make_timeout_time_ms(1000);

    printf("Sending 10 packets/s. Keys: 0-3 = PA level, r = reset totals.\n");

    while (true) {
        int c = getchar_timeout_us(0);
        if (c >= '0' && c <= '3') {
            pa = c - '0';
            nrf24_set_pa_level(pa);
            printf(">> PA level %s dBm\n", pa_names[pa]);
        } else if (c == 'r') {
            total_sent = total_ok = 0;
            printf(">> totals reset\n");
        }

        if (time_reached(next_send)) {
            next_send = delayed_by_ms(next_send, 100);
            ping_t ping = {.seq = seq++, .tx_ms = to_ms_since_boot(get_absolute_time())};
            uint8_t retries = 0;
            bool ok = nrf24_send(&ping, sizeof(ping), &retries);

            sec_sent++;
            total_sent++;
            if (ok) {
                sec_ok++;
                total_ok++;
                sec_retries += retries;
                gpio_put(led, 1);
            }
            // Drain ACK payloads (receiver's counter)
            while (nrf24_available()) {
                pong_t pong;
                if (nrf24_read(&pong, sizeof(pong)) == sizeof(pong)) last_pong = pong.rx_count;
            }
        }
        if (absolute_time_diff_us(next_send, get_absolute_time()) > -80000) {
            gpio_put(led, 0);  // LED on for ~20 ms after each ACKed packet
        }

        if (time_reached(next_report)) {
            next_report = delayed_by_ms(next_report, 1000);
            float loss = total_sent ? 100.0f * (total_sent - total_ok) / total_sent : 0;
            printf("[PA %3s dBm] this second: %2lu/%2lu ACKed, avg retries %.1f | "
                   "total loss %.1f%% (%lu/%lu) | receiver count %lu\n",
                   pa_names[pa], (unsigned long)sec_ok, (unsigned long)sec_sent,
                   sec_ok ? (float)sec_retries / sec_ok : 0.0f, loss,
                   (unsigned long)(total_sent - total_ok), (unsigned long)total_sent,
                   (unsigned long)last_pong);
            sec_sent = sec_ok = sec_retries = 0;
        }
    }
}

static void run_receiver(void) {
    uint32_t rx_count = 0, sec_rx = 0, sec_strong = 0, missed = 0;
    uint32_t expected_seq = 0;
    bool first = true;
    absolute_time_t next_report = make_timeout_time_ms(1000);
    absolute_time_t last_rx = get_absolute_time();

    nrf24_start_listening();
    pong_t pong = {0};
    nrf24_queue_ack_payload(&pong, sizeof(pong));
    printf("Listening on channel %d...\n", LINK_CHANNEL);

    while (true) {
        if (nrf24_available()) {
            ping_t ping;
            if (nrf24_read(&ping, sizeof(ping)) == sizeof(ping)) {
                if (!first && ping.seq != expected_seq) {
                    // seq restarts at 0 when the remote reboots
                    if (ping.seq > expected_seq) missed += ping.seq - expected_seq;
                }
                first = false;
                expected_seq = ping.seq + 1;
                rx_count++;
                sec_rx++;
                if (nrf24_signal_strong()) sec_strong++;
                last_rx = get_absolute_time();
                gpio_xor_mask(1u << led);

                pong.rx_count = rx_count;
                nrf24_queue_ack_payload(&pong, sizeof(pong));
            }
        }

        if (time_reached(next_report)) {
            next_report = delayed_by_ms(next_report, 1000);
            if (absolute_time_diff_us(last_rx, get_absolute_time()) > 2000000) {
                printf("no packets for %llu s - is the REMOTE running?\n",
                       absolute_time_diff_us(last_rx, get_absolute_time()) / 1000000);
                gpio_put(led, 0);
            } else {
                printf("this second: %2lu packets, %2lu strong (>-64 dBm) | "
                       "total %lu, missed %lu\n",
                       (unsigned long)sec_rx, (unsigned long)sec_strong,
                       (unsigned long)rx_count, (unsigned long)missed);
            }
            sec_rx = sec_strong = 0;
        }
    }
}

int main(void) {
    stdio_init_all();
    gpio_init(led);
    gpio_set_dir(led, GPIO_OUT);

    for (int i = 0; i < 50 && !stdio_usb_connected(); i++) {
        sleep_ms(100);
    }

    board_role_t role = board_role();
    printf("\n=== pico-leds step 4: ping-pong on %s board ===\n", board_role_name(role));

    nrf24_init();
    nrf24_setup(LINK_CHANNEL, LINK_ADDR);

    if (role == BOARD_REMOTE) {
        run_remote();
    } else if (role == BOARD_RECEIVER) {
        run_receiver();
    }

    printf("Unknown board - add its ID to common/board_id.c\n");
    while (true) {
        gpio_xor_mask(1u << led);
        sleep_ms(1000);
    }
}
