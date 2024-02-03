/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef BOARD_UART_2_H
#define BOARD_UART_2_H

#include <stdint.h>

typedef struct std_error std_error_t;

int board_uart_2_init (std_error_t * const error);
int board_uart_2_deinit (std_error_t * const error);

int board_uart_2_write (const uint8_t *data,
                        uint16_t data_size,
                        uint32_t timeout_ms,
                        std_error_t * const error);

#endif // BOARD_UART_2_H
