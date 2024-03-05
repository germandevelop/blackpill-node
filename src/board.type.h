/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#ifndef BOARD_TYPE_H
#define BOARD_TYPE_H

#include <stdint.h>

typedef enum board_remote_button
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

} board_remote_button_t;

typedef enum board_led_color
{
    NO_COLOR = 0,
    GREEN_COLOR,
    BLUE_COLOR,
    RED_COLOR

} board_led_color_t;

typedef struct photoresistor_data
{
    float voltage_V;
    uint32_t resistance_Ohm;

} photoresistor_data_t;

#endif // BOARD_TYPE_H
