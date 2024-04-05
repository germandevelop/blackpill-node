/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "board.gpio_c.h"

#include "stm32f4xx_hal.h"


void board_gpio_c_init ()
{
    // GPIO Ports Clock Enable
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // Configure GPIO pin
    // PC13 - W5500 Slave Select
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin     = GPIO_PIN_13;
    GPIO_InitStruct.Mode    = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull    = GPIO_NOPULL;
    GPIO_InitStruct.Speed   = GPIO_SPEED_FREQ_MEDIUM;

    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    return;
}

void board_gpio_c_pin_13_set ()
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    return;
}

void board_gpio_c_pin_13_reset ()
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

    return;
}

void board_gpio_c_deinit ()
{
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_13);

    return;
}