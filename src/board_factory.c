/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#include "board_factory.h"

#include "board_T01.h"


void board_factory_build_setup (board_setup_t * const setup)
{
    setup->node_id = NODE_T01;

    setup->init_extension_callback = board_T01_init;

    setup->is_remote_control_enabled_callback   = board_T01_is_remote_control_enabled;
    setup->is_lightning_on_callback             = board_T01_is_lightning_on;

    setup->process_remote_button_callback       = board_T01_process_remote_button;
    setup->process_photoresistor_data_callback  = board_T01_process_photoresistor_data;
    setup->process_msg_callback                 = board_T01_process_node_msg;

    return;
}
