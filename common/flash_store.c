#include "flash_store.h"

#include <string.h>
#include "hardware/flash.h"
#include "pico/flash.h"

#define STORE_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

typedef struct {
    uint32_t magic;
    uint32_t len;
    uint32_t checksum;
    uint8_t data[FLASH_STORE_MAX];
} store_t;

_Static_assert(sizeof(store_t) <= FLASH_PAGE_SIZE, "store must fit one flash page");

static uint32_t checksum(uint32_t magic, const uint8_t *p, size_t len) {
    uint32_t sum = magic ^ len;
    for (size_t i = 0; i < len; i++) sum = sum * 31 + p[i];
    return sum;
}

bool flash_store_load(uint32_t magic, void *data, size_t len) {
    const store_t *s = (const store_t *)(XIP_BASE + STORE_OFFSET);
    if (len > FLASH_STORE_MAX || s->magic != magic || s->len != len ||
        s->checksum != checksum(magic, s->data, len)) {
        return false;
    }
    memcpy(data, s->data, len);
    return true;
}

static void write_sector(void *page) {
    flash_range_erase(STORE_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(STORE_OFFSET, page, FLASH_PAGE_SIZE);
}

bool flash_store_save(uint32_t magic, const void *data, size_t len) {
    if (len > FLASH_STORE_MAX) return false;
    static uint8_t page[FLASH_PAGE_SIZE];
    memset(page, 0xFF, sizeof(page));

    store_t *s = (store_t *)page;
    s->magic = magic;
    s->len = len;
    memcpy(s->data, data, len);
    s->checksum = checksum(magic, s->data, len);

    // Code can't run from flash while it's being erased: this disables interrupts
    return flash_safe_execute(write_sector, page, 100) == PICO_OK;
}
