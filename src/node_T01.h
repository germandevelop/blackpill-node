/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#ifndef NODE_T01_H
#define NODE_T01_H

#define NODE_T01_LIGHT_DURATION_MS      (30U * 1000U)       // 30 seconds
#define NODE_T01_DISPLAY_DURATION_MS    (30U * 1000U)       // 30 seconds
#define NODE_T01_INTRUSION_DURATION_MS  (30U * 1000U)       // 30 seconds
#define NODE_T01_LUMINOSITY_PERIOD_MS   (1U * 10U * 1000U)  // 5 min
#define NODE_T01_HUMIDITY_PERIOD_MS     (2U * 60U * 1000U)  // 2 min
#define NODE_T01_DOOR_STATE_PERIOD_MS   (2U * 60U * 1000U)  // 2 min

#define NODE_T01_DARKNESS_LEVEL_LUX 11.5F
#define NODE_T01_HIGH_TEMPERATURE_C 25.0F
#define NODE_T01_LOW_TEMPERATURE_C  15.0F

#include <stdint.h>
#include <stdbool.h>

#include "node.type.h"
#include "node/node.command.h"
#include "board.type.h"

typedef struct node_T01 node_T01_t;

typedef struct node_T01_state
{
    board_led_color_t status_led_color;
    bool is_light_on;
    bool is_display_on;
    bool is_warning_led_on;

    bool is_msg_to_send;

} node_T01_state_t;

typedef struct node_T01_luminosity
{
    float lux;
    bool is_valid;

} node_T01_luminosity_t;

typedef struct node_T01_humidity
{
    float pressure_hPa;
    float temperature_C;
    float humidity_pct;
    bool is_valid;

} node_T01_humidity_t;


#ifdef __cplusplus
extern "C" {
#endif

void node_T01_init (node_T01_t * const self);

void node_T01_get_id (node_T01_t const * const self, node_id_t * const id);

void node_T01_get_state (node_T01_t * const self,
                        node_T01_state_t * const state,
                        uint32_t time_ms);

void node_T01_process_luminosity (  node_T01_t * const self,
                                    node_T01_luminosity_t const * const data,
                                    uint32_t * const next_time_ms);

void node_T01_process_humidity (node_T01_t * const self,
                                node_T01_humidity_t const * const data,
                                uint32_t * const next_time_ms);

void node_T01_process_door_state (  node_T01_t * const self,
                                    bool is_door_open,
                                    uint32_t * const next_time_ms);

void node_T01_process_remote_button (node_T01_t * const self,
                                    board_remote_button_t remote_button);

void node_T01_process_movement (node_T01_t * const self,
                                uint32_t time_ms);

void node_T01_process_msg (node_T01_t * const self,
                            node_msg_t const * const rcv_msg,
                            uint32_t time_ms);

void node_T01_get_display_data (node_T01_t const * const self,
                                node_T01_humidity_t * const data,
                                uint32_t * const disable_time_ms);

void node_T01_get_msg ( node_T01_t * const self,
                        node_msg_t *msg,
                        bool * const is_msg_valid);

#ifdef __cplusplus
}
#endif



// Private
typedef struct node_T01
{
    node_id_t id;

    node_T01_state_t state;

    node_mode_id_t mode;
    bool is_dark;

    uint32_t light_start_time_ms;
    uint32_t display_start_time_ms;
    uint32_t intrusion_start_time_ms;

    node_T01_humidity_t humidity;
    bool is_door_open;
    bool is_warning_enabled;

    node_msg_t send_msg_buffer[8];
    size_t send_msg_buffer_size;

} node_T01_t;

#endif // NODE_T01_H
