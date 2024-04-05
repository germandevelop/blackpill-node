/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#ifndef BOARD_T01_H
#define BOARD_T01_H

#include "board_factory.type.h"

int board_T01_init (board_extension_config_t const * const init_config, std_error_t * const error);

void board_T01_is_remote_control_enabled (bool * const is_remote_control_enabled);

void board_T01_is_lightning_on (bool * const is_lightning_on);
void board_T01_process_remote_button (board_remote_button_t remote_button);
void board_T01_process_photoresistor_data (photoresistor_data_t const * const data, uint32_t * const next_time_ms);
void board_T01_process_node_msg (node_msg_t const * const rcv_msg);

#endif // BOARD_T01_H
