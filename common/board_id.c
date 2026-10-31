#include "board_id.h"

#include <string.h>
#include "pico/unique_id.h"

#define REMOTE_ID   "E664B012CF471B37"
#define RECEIVER_ID "E664B012CF781437"

board_role_t board_role(void) {
    char id[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    pico_get_unique_board_id_string(id, sizeof(id));
    if (strcmp(id, REMOTE_ID) == 0) return BOARD_REMOTE;
    if (strcmp(id, RECEIVER_ID) == 0) return BOARD_RECEIVER;
    return BOARD_UNKNOWN;
}

const char *board_role_name(board_role_t role) {
    switch (role) {
        case BOARD_REMOTE:   return "REMOTE";
        case BOARD_RECEIVER: return "RECEIVER";
        default:             return "UNKNOWN";
    }
}
