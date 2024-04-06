/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#include "board_factory.h"

#include "board_T01.h"
#include "board_B02.h"


static void board_factory_build_T01_setup (board_setup_t * const setup);
static void board_factory_build_B02_setup (board_setup_t * const setup);

void board_factory_build_setup (board_setup_t * const setup)
{
    board_factory_build_T01_setup(setup);

    return;
}


void board_factory_build_T01_setup (board_setup_t * const setup)
{
    setup->node_id = NODE_T01;

    setup->init_extension_callback = board_T01_init;

    setup->is_remote_control_enabled_callback = board_T01_is_remote_control_enabled;

    setup->disable_lightning_callback           = board_T01_disable_lightning;
    setup->process_remote_button_callback       = board_T01_process_remote_button;
    setup->process_photoresistor_data_callback  = board_T01_process_photoresistor_data;
    setup->process_msg_callback                 = board_T01_process_node_msg;

    return;
}

void board_factory_build_B02_setup (board_setup_t * const setup)
{
    setup->node_id = NODE_B02;

    setup->init_extension_callback = board_B02_init;

    setup->is_remote_control_enabled_callback = board_B02_is_remote_control_enabled;

    setup->disable_lightning_callback           = board_B02_disable_lightning;
    setup->process_remote_button_callback       = board_B02_process_remote_button;
    setup->process_photoresistor_data_callback  = board_B02_process_photoresistor_data;
    setup->process_msg_callback                 = board_B02_process_node_msg;

    return;
}
