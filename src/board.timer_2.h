/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef BOARD_TIMER_2_H
#define BOARD_TIMER_2_H

#include <stdint.h>

typedef struct std_error std_error_t;

typedef void (*board_timer_2_ic_isr_callback_t) (uint32_t captured_value);

typedef struct board_timer_2_config
{
    board_timer_2_ic_isr_callback_t ic_isr_callback;

} board_timer_2_config_t;

int board_timer_2_init (board_timer_2_config_t const * const init_config, std_error_t * const error);
void board_timer_2_deinit ();

int board_timer_2_start_channel_2 (std_error_t * const error);
void board_timer_2_stop_channel_2 ();

#endif // BOARD_TIMER_2_H
