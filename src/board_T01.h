/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#ifndef BOARD_T01_H
#define BOARD_T01_H

#include <stdint.h>

#include "board.types.h"

typedef struct mcp23017_expander mcp23017_expander_t;
typedef struct w25q32bv_flash w25q32bv_flash_t;
typedef struct node_msg node_msg_t;
typedef struct std_error std_error_t;

typedef int (*board_T01_set_led_color_callback_t) (board_led_color_t led_color, std_error_t * const error);
typedef int (*board_T01_get_luminosity_callback_t) (uint32_t * const luminosity_adc, std_error_t * const error);
typedef int (*board_T01_send_node_msg_callback_t) (node_msg_t const * const send_msg, std_error_t * const error);

typedef struct board_T01_config
{
    mcp23017_expander_t *mcp23017_expander;
    w25q32bv_flash_t *w25q32bv_flash;

    board_T01_set_led_color_callback_t set_led_color_callback;
    board_T01_get_luminosity_callback_t get_luminosity_callback;
    board_T01_send_node_msg_callback_t send_node_msg_callback;

} board_T01_config_t;

int board_T01_init (board_T01_config_t const * const init_config, std_error_t * const error);

void board_T01_remote_control_ISR (board_remote_button_t remote_button);

void board_T01_receive_node_msg (node_msg_t const * const rcv_msg);

#endif // BOARD_T01_H
