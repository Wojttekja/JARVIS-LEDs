// Small persistent blob in the last 4 KB flash sector (magic + checksum protected).
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FLASH_STORE_MAX 240  // bytes of payload

// Copies the stored blob into data. Returns false (data untouched) if nothing
// valid with this magic and length is stored.
bool flash_store_load(uint32_t magic, void *data, size_t len);

// Erases the sector and writes the blob. Interrupts are off for ~50 ms.
bool flash_store_save(uint32_t magic, const void *data, size_t len);
