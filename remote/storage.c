#include "storage.h"

#include "flash_store.h"

#define STORAGE_MAGIC 0x4C454453u  // "LEDS"

bool storage_load(rgb_t slots[STORAGE_COLOR_SLOTS]) {
    return flash_store_load(STORAGE_MAGIC, slots, sizeof(rgb_t) * STORAGE_COLOR_SLOTS);
}

bool storage_save(const rgb_t slots[STORAGE_COLOR_SLOTS]) {
    return flash_store_save(STORAGE_MAGIC, slots, sizeof(rgb_t) * STORAGE_COLOR_SLOTS);
}
