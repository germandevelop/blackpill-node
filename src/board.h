/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

typedef struct std_error std_error_t;

typedef void (*board_refresh_watchdog_callback_t) ();

typedef struct board_config
{
    board_refresh_watchdog_callback_t refresh_watchdog_callback;
    uint32_t watchdog_timeout_ms;

} board_config_t;

int board_init (board_config_t const * const init_config, std_error_t * const error);

#endif // BOARD_H
