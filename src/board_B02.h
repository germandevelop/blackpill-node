/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#ifndef BOARD_B02_H
#define BOARD_B02_H

#include "board_factory.type.h"

int board_B02_init (board_extension_config_t const * const init_config, std_error_t * const error);

void board_B02_is_remote_control_enabled (bool * const is_remote_control_enabled);

void board_B02_disable_lightning (uint32_t period_ms, bool * const is_lightning_disabled);
void board_B02_process_remote_button (board_remote_button_t remote_button);
void board_B02_process_photoresistor_data (photoresistor_data_t const * const data, uint32_t * const next_time_ms);
void board_B02_process_node_msg (node_msg_t const * const rcv_msg);

#endif // BOARD_B02_H
