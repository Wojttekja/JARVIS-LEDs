// Saved color slots (K7-K9) in the last flash sector of the remote.
#pragma once

#include <stdbool.h>
#include "protocol.h"

#define STORAGE_COLOR_SLOTS 3

// Fills slots from flash. Returns false (slots untouched) if nothing valid is stored.
bool storage_load(rgb_t slots[STORAGE_COLOR_SLOTS]);
bool storage_save(const rgb_t slots[STORAGE_COLOR_SLOTS]);
