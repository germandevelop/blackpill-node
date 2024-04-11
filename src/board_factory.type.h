/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#ifndef BOARD_FACTORY_TYPE_H
#define BOARD_FACTORY_TYPE_H

#include <stdbool.h>

#include "board.type.h"
#include "node.type.h"

typedef struct mcp23017_expander mcp23017_expander_t;
typedef struct storage storage_t;
typedef struct std_error std_error_t;

typedef void (*board_extension_lock_i2c_1_callback_t) ();
typedef void (*board_extension_update_status_led_callback_t) (board_led_color_t led_color);
typedef int (*board_extension_send_node_msg_callback_t) (node_msg_t const * const send_msg, std_error_t * const error);

typedef struct board_extension_config
{
    mcp23017_expander_t *mcp23017_expander;
    storage_t *storage;

    board_extension_lock_i2c_1_callback_t lock_i2c_1_callback;
    board_extension_lock_i2c_1_callback_t unlock_i2c_1_callback;

    board_extension_update_status_led_callback_t update_status_led_callback;
    board_extension_send_node_msg_callback_t send_node_msg_callback;

} board_extension_config_t;


typedef int (*board_setup_init_extension_callback_t) (board_extension_config_t const * const config, std_error_t * const error);
typedef void (*board_setup_is_remote_control_enabled_callback_t) (bool * const is_remote_control_enabled);
typedef void (*board_setup_disable_lightning_callback_t) (uint32_t period_ms, bool * const is_lightning_disabled);
typedef void (*board_setup_process_remote_button_callback_t) (board_remote_button_t remote_button);
typedef void (*board_setup_process_photoresistor_data_callback_t) (photoresistor_data_t const * const data, uint32_t * const next_time_ms);
typedef void (*board_setup_process_msg_callback_t) (node_msg_t const * const msg);

typedef struct board_setup
{
    uint8_t unique_id[12];

    node_id_t node_id;

    board_setup_init_extension_callback_t init_extension_callback;

    board_setup_is_remote_control_enabled_callback_t is_remote_control_enabled_callback;

    board_setup_disable_lightning_callback_t disable_lightning_callback;
    board_setup_process_remote_button_callback_t process_remote_button_callback;
    board_setup_process_photoresistor_data_callback_t process_photoresistor_data_callback;
    board_setup_process_msg_callback_t process_msg_callback;

} board_setup_t;

#endif // BOARD_FACTORY_TYPE_H
