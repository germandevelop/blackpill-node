/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#include "board_factory.h"

#include <string.h>

#include "board_T01.h"
#include "board_B02.h"


#define STM32_UUID ((uint8_t*)0x1FFF7A10)

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


static void board_factory_build_T01_setup (board_setup_t * const setup);
static void board_factory_build_B02_setup (board_setup_t * const setup);
static void board_factory_build_unknown_setup (board_setup_t * const setup);


void board_factory_build_setup (board_setup_t * const setup)
{
    for (size_t i = 0U; i < ARRAY_SIZE(setup->unique_id); ++i)
    {
        setup->unique_id[i] = STM32_UUID[i];
    }

    const uint8_t unique_id_T01[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    const uint8_t unique_id_B02[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    if (memcmp((const void*)(setup->unique_id), (const void*)(unique_id_T01), sizeof(setup->unique_id)) == 0)
    {
        board_factory_build_T01_setup(setup);
    }

    else if (memcmp((const void*)(setup->unique_id), (const void*)(unique_id_B02), sizeof(setup->unique_id)) == 0)
    {
        board_factory_build_B02_setup(setup);
    }

    else
    {
        board_factory_build_T01_setup(setup);
        //board_factory_build_unknown_setup(setup);
    }

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

void board_factory_build_unknown_setup (board_setup_t * const setup)
{
    setup->node_id = NODE_BROADCAST;

    setup->init_extension_callback = NULL;

    setup->is_remote_control_enabled_callback = NULL;

    setup->disable_lightning_callback           = NULL;
    setup->process_remote_button_callback       = NULL;
    setup->process_photoresistor_data_callback  = NULL;
    setup->process_msg_callback                 = NULL;

    return;
}
