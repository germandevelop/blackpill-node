/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#ifndef NODE_B02_H
#define NODE_B02_H

#define NODE_B02_LIGHT_DURATION_MS      (30U * 1000U)       // 30 seconds
#define NODE_B02_DISPLAY_DURATION_MS    (30U * 1000U)       // 30 seconds
#define NODE_B02_INTRUSION_DURATION_MS  (30U * 1000U)       // 30 seconds
#define NODE_B02_LUMINOSITY_PERIOD_MS   (2U * 60U * 1000U)  // 2 min
#define NODE_B02_TEMPERATURE_PERIOD_MS  (2U * 60U * 1000U)  // 2 min

#define NODE_B02_DARKNESS_LEVEL_LUX 5.5F

#include <stdint.h>
#include <stdbool.h>

#include "node.type.h"
#include "node/node.command.h"
#include "board.type.h"

typedef struct node_B02 node_B02_t;
typedef struct std_error std_error_t;


typedef struct node_B02_light_strip
{
    bool is_white_on;
    bool is_blue_green_on;
    bool is_red_on;

} node_B02_light_strip_t;

typedef struct node_B02_state
{
    board_led_color_t status_led_color;
    bool is_display_on;
    bool is_front_pir_on;
    node_B02_light_strip_t light_strip;
    bool is_veranda_light_on;
    bool is_front_light_on;
    bool is_buzzer_on;

    bool is_msg_to_send;

} node_B02_state_t;

typedef struct node_B02_luminosity
{
    float lux;
    bool is_valid;

} node_B02_luminosity_t;

typedef struct node_B02_temperature
{
    float pressure_hPa;
    float temperature_C;
    bool is_valid;

} node_B02_temperature_t;


#ifdef __cplusplus
extern "C" {
#endif

void node_B02_init (node_B02_t * const self);

void node_B02_get_state (node_B02_t * const self,
                        node_B02_state_t * const state,
                        uint32_t time_ms);

void node_B02_process_luminosity (  node_B02_t * const self,
                                    node_B02_luminosity_t const * const data,
                                    uint32_t * const next_time_ms);

void node_B02_process_temperature ( node_B02_t * const self,
                                    node_B02_temperature_t const * const data,
                                    uint32_t * const next_time_ms);

void node_B02_process_remote_button (node_B02_t * const self,
                                    board_remote_button_t remote_button);

void node_B02_process_door_movement (node_B02_t * const self,
                                    uint32_t time_ms);

void node_B02_process_front_movement (  node_B02_t * const self,
                                        uint32_t time_ms);

void node_B02_process_veranda_movement (node_B02_t * const self,
                                        uint32_t time_ms);

void node_B02_process_rcv_msg ( node_B02_t * const self,
                                node_msg_t const * const rcv_msg,
                                uint32_t time_ms);

void node_B02_get_display_data (node_B02_t const * const self,
                                node_B02_temperature_t * const data,
                                uint32_t * const disable_time_ms);

int node_B02_get_msg (  node_B02_t * const self,
                        node_msg_t *msg,
                        std_error_t * const error);

#ifdef __cplusplus
}
#endif



// Private
typedef struct node_B02
{
    node_id_t id;

    node_B02_state_t state;

    node_mode_id_t mode;
    bool is_dark;

    uint32_t light_start_time_ms;
    uint32_t display_start_time_ms;
    uint32_t intrusion_start_time_ms;

    node_B02_temperature_t temperature;

    node_msg_t send_msg_buffer[8];
    size_t send_msg_buffer_size;

} node_B02_t;

#endif // NODE_B02_H
