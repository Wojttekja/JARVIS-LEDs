// Step 3: nRF24L01+ probe - prove SPI wiring works by reading/writing registers.
// Runs the test every 2 s so you can wiggle wires and watch it pass/fail.
// LED: solid = PASS, fast blink = FAIL.
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "board_id.h"
#include "nrf24.h"

static int failures;

static void check(const char *what, uint8_t got, uint8_t want) {
    bool ok = got == want;
    if (!ok) failures++;
    printf("  %-28s got 0x%02x  want 0x%02x  %s\n", what, got, want, ok ? "ok" : "FAIL");
}

static bool run_probe(void) {
    failures = 0;

    // 1. STATUS via NOP. 0x00/0xFF means nothing is answering on MISO.
    uint8_t status = nrf24_status();
    printf("  STATUS (NOP)                 0x%02x", status);
    if (status == 0x00 || status == 0xFF) {
        printf("  FAIL - no response: check VCC/GND, MISO, CSN, SCK\n");
        return false;
    }
    printf("\n");

    // 2. Informational dump (values persist across Pico resets while powered)
    printf("  CONFIG=0x%02x EN_AA=0x%02x SETUP_AW=0x%02x RF_CH=0x%02x RF_SETUP=0x%02x\n",
           nrf24_read_reg(NRF24_REG_CONFIG), nrf24_read_reg(NRF24_REG_EN_AA),
           nrf24_read_reg(NRF24_REG_SETUP_AW), nrf24_read_reg(NRF24_REG_RF_CH),
           nrf24_read_reg(NRF24_REG_RF_SETUP));

    // 3. RF_CH write/read-back with bit patterns (7-bit register)
    const uint8_t patterns[] = {0x2A, 0x55, 0x7F, 0x00};
    for (size_t i = 0; i < sizeof(patterns); i++) {
        char label[32];
        snprintf(label, sizeof(label), "RF_CH write 0x%02x", patterns[i]);
        nrf24_write_reg(NRF24_REG_RF_CH, patterns[i]);
        check(label, nrf24_read_reg(NRF24_REG_RF_CH), patterns[i]);
    }

    // 4. Multi-byte register: 5-byte TX address
    const uint8_t addr[5] = {0xA5, 0x5A, 0xC3, 0x3C, 0x96};
    uint8_t back[5] = {0};
    nrf24_write_regs(NRF24_REG_TX_ADDR, addr, 5);
    nrf24_read_regs(NRF24_REG_TX_ADDR, back, 5);
    bool addr_ok = memcmp(addr, back, 5) == 0;
    if (!addr_ok) failures++;
    printf("  %-28s %02x%02x%02x%02x%02x  %s\n", "TX_ADDR 5-byte read-back",
           back[0], back[1], back[2], back[3], back[4], addr_ok ? "ok" : "FAIL");

    // 5. Is it an nRF24L01+ ("plus")? Only the + version keeps the 250 kbps bit.
    nrf24_write_reg(NRF24_REG_RF_SETUP, 0x0F | NRF24_RF_DR_LOW);
    bool plus = nrf24_read_reg(NRF24_REG_RF_SETUP) & NRF24_RF_DR_LOW;
    printf("  %-28s %s\n", "250 kbps support (L01+)", plus ? "yes" : "NO - old nRF24L01 or fake");

    // Restore datasheet defaults
    const uint8_t default_addr[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    nrf24_write_reg(NRF24_REG_RF_CH, 0x02);
    nrf24_write_reg(NRF24_REG_RF_SETUP, 0x0F);
    nrf24_write_regs(NRF24_REG_TX_ADDR, default_addr, 5);

    return failures == 0;
}

int main(void) {
    stdio_init_all();
    const uint led = PICO_DEFAULT_LED_PIN;
    gpio_init(led);
    gpio_set_dir(led, GPIO_OUT);

    for (int i = 0; i < 50 && !stdio_usb_connected(); i++) {
        sleep_ms(100);
    }
    const char *role = board_role_name(board_role());
    printf("\n=== pico-leds step 3: nRF24 probe on %s board ===\n", role);

    nrf24_init();

    for (uint32_t run = 1;; run++) {
        printf("\n--- probe run %lu on %s ---\n", (unsigned long)run, role);
        bool pass = run_probe();
        printf("RESULT: %s\n", pass ? "PASS" : "FAIL");

        // LED: solid on PASS, fast blink on FAIL, for 2 s
        for (int i = 0; i < 20; i++) {
            gpio_put(led, pass ? 1 : (i & 1));
            sleep_ms(100);
        }
    }
}
