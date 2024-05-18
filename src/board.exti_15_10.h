/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#ifndef BOARD_EXTI_15_10_H
#define BOARD_EXTI_15_10_H

#include <stdbool.h>

typedef struct std_error std_error_t;

typedef void (*board_exti_15_10_callback_t) ();

typedef struct board_exti_15_10_config
{
    board_exti_15_10_callback_t exti_12_callback;
    board_exti_15_10_callback_t exti_15_callback;

} board_exti_15_10_config_t;

int board_exti_15_10_init (board_exti_15_10_config_t const * const init_config, std_error_t * const error);
int board_exti_15_10_deinit (std_error_t * const error);

void board_exti_15_10_get_12 (bool * const is_high);

#endif // BOARD_EXTI_15_10_H
