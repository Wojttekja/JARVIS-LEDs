#include "nrf24.h"

#include <string.h>

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

#define NRF24_SPI      spi0
#define NRF24_SPI_HZ   (4 * 1000 * 1000)  // chip allows 10 MHz; 4 MHz is safe on jumper wires

#define CMD_R_REGISTER    0x00
#define CMD_W_REGISTER    0x20
#define CMD_R_RX_PAYLOAD  0x61
#define CMD_W_TX_PAYLOAD  0xA0
#define CMD_FLUSH_TX      0xE1
#define CMD_FLUSH_RX      0xE2
#define CMD_R_RX_PL_WID   0x60
#define CMD_W_ACK_PAYLOAD 0xA8  // | pipe
#define CMD_NOP           0xFF

// CONFIG bits
#define CONFIG_PRIM_RX (1u << 0)
#define CONFIG_PWR_UP  (1u << 1)
#define CONFIG_CRCO    (1u << 2)  // 2-byte CRC
#define CONFIG_EN_CRC  (1u << 3)

// STATUS bits
#define STATUS_MAX_RT (1u << 4)
#define STATUS_TX_DS  (1u << 5)
#define STATUS_RX_DR  (1u << 6)

// FIFO_STATUS bits
#define FIFO_RX_EMPTY (1u << 0)
#define FIFO_TX_EMPTY (1u << 4)

// FEATURE bits
#define FEATURE_EN_ACK_PAY (1u << 1)
#define FEATURE_EN_DPL     (1u << 2)

#define RF_SETUP_PWR_MASK 0x06

static inline void csn_low(void)  { gpio_put(NRF24_PIN_CSN, 0); }
static inline void csn_high(void) { gpio_put(NRF24_PIN_CSN, 1); }

void nrf24_init(void) {
    spi_init(NRF24_SPI, NRF24_SPI_HZ);  // mode 0, MSB first (SDK default)
    gpio_set_function(NRF24_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(NRF24_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(NRF24_PIN_MISO, GPIO_FUNC_SPI);

    gpio_init(NRF24_PIN_CSN);
    gpio_set_dir(NRF24_PIN_CSN, GPIO_OUT);
    csn_high();

    gpio_init(NRF24_PIN_CE);
    gpio_set_dir(NRF24_PIN_CE, GPIO_OUT);
    gpio_put(NRF24_PIN_CE, 0);

    gpio_init(NRF24_PIN_IRQ);
    gpio_set_dir(NRF24_PIN_IRQ, GPIO_IN);
    gpio_pull_up(NRF24_PIN_IRQ);  // IRQ is active-low, open-drain-ish

    sleep_ms(100);  // power-on reset time (datasheet: 100 ms)
}

void nrf24_read_regs(uint8_t reg, uint8_t *buf, uint8_t len) {
    uint8_t cmd = CMD_R_REGISTER | (reg & 0x1F);
    csn_low();
    spi_write_blocking(NRF24_SPI, &cmd, 1);
    spi_read_blocking(NRF24_SPI, CMD_NOP, buf, len);
    csn_high();
}

void nrf24_write_regs(uint8_t reg, const uint8_t *buf, uint8_t len) {
    uint8_t cmd = CMD_W_REGISTER | (reg & 0x1F);
    csn_low();
    spi_write_blocking(NRF24_SPI, &cmd, 1);
    spi_write_blocking(NRF24_SPI, buf, len);
    csn_high();
}

uint8_t nrf24_read_reg(uint8_t reg) {
    uint8_t v;
    nrf24_read_regs(reg, &v, 1);
    return v;
}

void nrf24_write_reg(uint8_t reg, uint8_t value) {
    nrf24_write_regs(reg, &value, 1);
}

uint8_t nrf24_status(void) {
    uint8_t cmd = CMD_NOP, status;
    csn_low();
    spi_write_read_blocking(NRF24_SPI, &cmd, &status, 1);
    csn_high();
    return status;
}

static void command(uint8_t cmd) {
    csn_low();
    spi_write_blocking(NRF24_SPI, &cmd, 1);
    csn_high();
}

static void write_payload(uint8_t cmd, const void *buf, uint8_t len) {
    csn_low();
    spi_write_blocking(NRF24_SPI, &cmd, 1);
    spi_write_blocking(NRF24_SPI, buf, len);
    csn_high();
}

static void clear_irq_flags(void) {
    nrf24_write_reg(NRF24_REG_STATUS, STATUS_RX_DR | STATUS_TX_DS | STATUS_MAX_RT);
}

void nrf24_setup(uint8_t channel, const uint8_t addr[5]) {
    gpio_put(NRF24_PIN_CE, 0);
    nrf24_write_reg(NRF24_REG_CONFIG, CONFIG_EN_CRC | CONFIG_CRCO);  // powered down
    nrf24_write_reg(NRF24_REG_EN_AA, 0x01);       // auto-ACK on pipe 0
    nrf24_write_reg(NRF24_REG_EN_RXADDR, 0x01);   // only pipe 0
    nrf24_write_reg(NRF24_REG_SETUP_AW, 0x03);    // 5-byte addresses
    // 1500 us between retries, 15 retries. At 250 kbps the wait must cover the
    // ACK incl. its payload: a 32-byte ACK payload takes ~1.3 ms on air.
    nrf24_write_reg(NRF24_REG_SETUP_RETR, 0x5F);
    nrf24_write_reg(NRF24_REG_RF_CH, channel & 0x7F);
    nrf24_write_reg(NRF24_REG_RF_SETUP, NRF24_RF_DR_LOW | RF_SETUP_PWR_MASK);  // 250 kbps, 0 dBm
    nrf24_write_regs(NRF24_REG_TX_ADDR, addr, 5);
    nrf24_write_regs(NRF24_REG_RX_ADDR_P0, addr, 5);  // PTX receives ACKs on pipe 0
    nrf24_write_reg(NRF24_REG_FEATURE, FEATURE_EN_DPL | FEATURE_EN_ACK_PAY);
    nrf24_write_reg(NRF24_REG_DYNPD, 0x01);       // dynamic payload length on pipe 0

    command(CMD_FLUSH_TX);
    command(CMD_FLUSH_RX);
    clear_irq_flags();

    nrf24_write_reg(NRF24_REG_CONFIG, CONFIG_EN_CRC | CONFIG_CRCO | CONFIG_PWR_UP);
    sleep_ms(5);  // crystal start-up (1.5 ms typ)
}

bool nrf24_config_intact(uint8_t channel) {
    uint8_t want_config = CONFIG_EN_CRC | CONFIG_CRCO | CONFIG_PWR_UP;
    return (nrf24_read_reg(NRF24_REG_CONFIG) & ~CONFIG_PRIM_RX & 0x7F) == want_config &&
           nrf24_read_reg(NRF24_REG_RF_CH) == (channel & 0x7F) &&
           (nrf24_read_reg(NRF24_REG_RF_SETUP) & NRF24_RF_DR_LOW) &&
           nrf24_read_reg(NRF24_REG_FEATURE) == (FEATURE_EN_DPL | FEATURE_EN_ACK_PAY) &&
           nrf24_read_reg(NRF24_REG_DYNPD) == 0x01;
}

void nrf24_set_pa_level(uint8_t level) {
    uint8_t rf = nrf24_read_reg(NRF24_REG_RF_SETUP) & ~RF_SETUP_PWR_MASK;
    nrf24_write_reg(NRF24_REG_RF_SETUP, rf | ((level & 0x03) << 1));
}

void nrf24_start_listening(void) {
    nrf24_write_reg(NRF24_REG_CONFIG, nrf24_read_reg(NRF24_REG_CONFIG) | CONFIG_PRIM_RX);
    clear_irq_flags();
    gpio_put(NRF24_PIN_CE, 1);
    sleep_us(130);  // RX settling
}

void nrf24_stop_listening(void) {
    gpio_put(NRF24_PIN_CE, 0);
    sleep_us(130);
    nrf24_write_reg(NRF24_REG_CONFIG, nrf24_read_reg(NRF24_REG_CONFIG) & ~CONFIG_PRIM_RX);
}

bool nrf24_send(const void *buf, uint8_t len, uint8_t *retries) {
    if (len > NRF24_MAX_PAYLOAD) len = NRF24_MAX_PAYLOAD;
    write_payload(CMD_W_TX_PAYLOAD, buf, len);

    gpio_put(NRF24_PIN_CE, 1);  // >10 us pulse starts the transmission
    sleep_us(15);
    gpio_put(NRF24_PIN_CE, 0);

    // Worst case: 16 attempts x (1500 us wait + airtime) ~ 35 ms
    absolute_time_t deadline = make_timeout_time_ms(60);
    uint8_t status;
    do {
        status = nrf24_status();
    } while (!(status & (STATUS_TX_DS | STATUS_MAX_RT)) && !time_reached(deadline));

    if (retries) *retries = nrf24_read_reg(NRF24_REG_OBSERVE_TX) & 0x0F;

    bool ok = status & STATUS_TX_DS;
    if (!ok) command(CMD_FLUSH_TX);  // drop the packet we gave up on
    // Leave RX_DR set if an ACK payload arrived; nrf24_available() reads FIFO anyway
    nrf24_write_reg(NRF24_REG_STATUS, STATUS_TX_DS | STATUS_MAX_RT);
    return ok;
}

bool nrf24_available(void) {
    return !(nrf24_read_reg(NRF24_REG_FIFO_STATUS) & FIFO_RX_EMPTY);
}

uint8_t nrf24_read(void *buf, uint8_t maxlen) {
    uint8_t cmd = CMD_R_RX_PL_WID, width;
    csn_low();
    spi_write_blocking(NRF24_SPI, &cmd, 1);
    spi_read_blocking(NRF24_SPI, CMD_NOP, &width, 1);
    csn_high();

    if (width == 0 || width > NRF24_MAX_PAYLOAD) {  // corrupt, datasheet says flush
        command(CMD_FLUSH_RX);
        nrf24_write_reg(NRF24_REG_STATUS, STATUS_RX_DR);
        return 0;
    }

    uint8_t tmp[NRF24_MAX_PAYLOAD];
    cmd = CMD_R_RX_PAYLOAD;
    csn_low();
    spi_write_blocking(NRF24_SPI, &cmd, 1);
    spi_read_blocking(NRF24_SPI, CMD_NOP, tmp, width);
    csn_high();
    nrf24_write_reg(NRF24_REG_STATUS, STATUS_RX_DR);

    uint8_t n = width < maxlen ? width : maxlen;
    memcpy(buf, tmp, n);
    return n;
}

bool nrf24_queue_ack_payload(const void *buf, uint8_t len) {
    if (!(nrf24_read_reg(NRF24_REG_FIFO_STATUS) & FIFO_TX_EMPTY)) return false;
    if (len > NRF24_MAX_PAYLOAD) len = NRF24_MAX_PAYLOAD;
    write_payload(CMD_W_ACK_PAYLOAD | 0, buf, len);
    return true;
}

bool nrf24_signal_strong(void) {
    return nrf24_read_reg(NRF24_REG_RPD) & 0x01;
}
