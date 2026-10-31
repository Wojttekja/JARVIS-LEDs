// Minimal nRF24L01+ driver over SPI0.
#pragma once

#include <stdbool.h>
#include <stdint.h>

// Wiring (same on both Picos)
#define NRF24_PIN_MISO 16
#define NRF24_PIN_CSN  17
#define NRF24_PIN_SCK  18
#define NRF24_PIN_MOSI 19
#define NRF24_PIN_CE   20
#define NRF24_PIN_IRQ  21

// Registers
#define NRF24_REG_CONFIG      0x00
#define NRF24_REG_EN_AA       0x01
#define NRF24_REG_EN_RXADDR   0x02
#define NRF24_REG_SETUP_AW    0x03
#define NRF24_REG_SETUP_RETR  0x04
#define NRF24_REG_RF_CH       0x05
#define NRF24_REG_RF_SETUP    0x06
#define NRF24_REG_STATUS      0x07
#define NRF24_REG_OBSERVE_TX  0x08
#define NRF24_REG_RPD         0x09
#define NRF24_REG_RX_ADDR_P0  0x0A
#define NRF24_REG_RX_ADDR_P1  0x0B
#define NRF24_REG_TX_ADDR     0x10
#define NRF24_REG_RX_PW_P0    0x11
#define NRF24_REG_RX_PW_P1    0x12
#define NRF24_REG_FIFO_STATUS 0x17
#define NRF24_REG_DYNPD       0x1C
#define NRF24_REG_FEATURE     0x1D

// RF_SETUP bits
#define NRF24_RF_DR_LOW  (1u << 5)  // 250 kbps (nRF24L01+ only)

#define NRF24_MAX_PAYLOAD 32

// Sets up SPI + CE/CSN pins. Leaves the radio in power-down.
void nrf24_init(void);

uint8_t nrf24_read_reg(uint8_t reg);
void nrf24_write_reg(uint8_t reg, uint8_t value);
void nrf24_read_regs(uint8_t reg, uint8_t *buf, uint8_t len);
void nrf24_write_regs(uint8_t reg, const uint8_t *buf, uint8_t len);
uint8_t nrf24_status(void);  // NOP command, returns STATUS

// Configures the link (250 kbps, 2-byte CRC, auto-ACK, 15 retries,
// dynamic payloads + ACK payloads, pipe 0) and powers the radio up in TX mode.
// Both sides must use the same channel and 5-byte address.
void nrf24_setup(uint8_t channel, const uint8_t addr[5]);

// PA level 0..3 = -18, -12, -6, 0 dBm
void nrf24_set_pa_level(uint8_t level);

void nrf24_start_listening(void);  // become receiver (PRX)
void nrf24_stop_listening(void);   // back to transmitter (PTX)

// Sends one packet and waits for the ACK. Returns true if acknowledged.
// *retries (optional) = retransmissions needed (0..15).
bool nrf24_send(const void *buf, uint8_t len, uint8_t *retries);

// True if a received packet (or an ACK payload, on the TX side) is waiting.
bool nrf24_available(void);

// Reads one packet into buf; returns its length (0 if none / corrupt).
uint8_t nrf24_read(void *buf, uint8_t maxlen);

// Receiver side: queue data to be sent back inside the next ACK.
// Only queues if the TX FIFO is empty, so at most one is pending.
bool nrf24_queue_ack_payload(const void *buf, uint8_t len);

// True if the last received packet was stronger than -64 dBm.
bool nrf24_signal_strong(void);
