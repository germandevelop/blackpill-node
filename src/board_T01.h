/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#ifndef BOARD_T01_H
#define BOARD_T01_H

#include <stdint.h>
#include <stdbool.h>

#include "board.types.h"

typedef struct mcp23017_expander mcp23017_expander_t;
typedef struct w25q32bv_flash w25q32bv_flash_t;
typedef struct node_msg node_msg_t;
typedef struct std_error std_error_t;

typedef void (*board_T01_update_status_led_callback_t) (board_led_color_t led_color);
typedef int (*board_T01_send_node_msg_callback_t) (node_msg_t const * const send_msg, std_error_t * const error);

typedef struct board_T01_config
{
    mcp23017_expander_t *mcp23017_expander;
    w25q32bv_flash_t *w25q32bv_flash;

    board_T01_update_status_led_callback_t update_status_led_callback;
    board_T01_send_node_msg_callback_t send_node_msg_callback;

} board_T01_config_t;

int board_T01_init (board_T01_config_t const * const init_config, std_error_t * const error);

void board_T01_process_remote_button (board_remote_button_t remote_button);
void board_T01_process_luminosity (uint32_t luminosity_adc, uint32_t * const next_time_ms);
void board_T01_get_lightning_status (bool * const is_lightning_on);
void board_T01_process_rcv_node_msg (node_msg_t const * const rcv_msg);

#endif // BOARD_T01_H
