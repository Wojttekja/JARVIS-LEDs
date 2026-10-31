// Identify which labeled Pico we're running on (by flash unique ID).
#pragma once

typedef enum {
    BOARD_UNKNOWN = 0,
    BOARD_REMOTE,
    BOARD_RECEIVER,
} board_role_t;

board_role_t board_role(void);
const char *board_role_name(board_role_t role);
