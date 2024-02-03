/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef NODE_H
#define NODE_H

#include <stdint.h>

typedef struct std_error std_error_t;

typedef enum node_remote_button
{
    ZERO_BUTTON     = 0,
    ONE_BUTTON      = 1,
    TWO_BUTTON      = 2,
    THREE_BUTTON    = 3,
    FOUR_BUTTON     = 4,
    FIVE_BUTTON     = 5,
    SIX_BUTTON      = 6,
    SEVEN_BUTTON    = 7,
    EIGHT_BUTTON    = 8,
    NINE_BUTTON     = 9,
    STAR_BUTTON     = 10,
    GRID_BUTTON     = 11,
    UP_BUTTON       = 12,
    LEFT_BUTTON     = 13,
    OK_BUTTON       = 14,
    RIGHT_BUTTON    = 15,
    DOWN_BUTTON     = 16,
    UNKNOWN_BUTTON  = 17

} node_remote_button_t;

typedef enum node_led_color
{
    NO_COLOR = 0,
    GREEN_COLOR,
    BLUE_COLOR,
    RED_COLOR

} node_led_color_t;

typedef int (*node_get_light_level_callback_t) (uint32_t * const light_level, std_error_t * const error);
typedef int (*node_set_led_color_callback_t) (node_led_color_t color, std_error_t * const error);

typedef struct node_config
{
    node_set_led_color_callback_t set_led_color_callback;
    node_get_light_level_callback_t get_light_level_callback;

} node_config_t;

int node_init (node_config_t const * const init_config, std_error_t * const error);

void node_remote_control_ISR (node_remote_button_t remote_button);

#endif // NODE_H
