/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef BOARD_TIMER_3_H
#define BOARD_TIMER_3_H

typedef struct std_error std_error_t;

int board_timer_3_init (std_error_t * const error);
void board_timer_3_deinit ();

void board_timer_3_enable_clock ();
void board_timer_3_disable_clock ();

int board_timer_3_start_channel_1 (std_error_t * const error);
int board_timer_3_start_channel_2 (std_error_t * const error);

#endif // BOARD_TIMER_3_H
