/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef BOARD_EXTI_1_H
#define BOARD_EXTI_1_H

typedef struct std_error std_error_t;

typedef void (*board_exti_1_callback_t) ();

int board_exti_1_init (board_exti_1_callback_t exti_1_callback, std_error_t * const error);
int board_exti_1_deinit (std_error_t * const error);

#endif // BOARD_EXTI_1_H
