/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#include "board.gpio_a.h"

#include "stm32f4xx_hal.h"


void board_gpio_a_init ()
{
    // GPIO Ports Clock Enable
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Configure GPIO pin
    // PA4 - W25Q Slave Select
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin     = GPIO_PIN_4;
    GPIO_InitStruct.Mode    = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull    = GPIO_NOPULL;
    GPIO_InitStruct.Speed   = GPIO_SPEED_FREQ_MEDIUM;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    return;
}

void board_gpio_a_pin_4_set ()
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

    return;
}

void board_gpio_a_pin_4_reset ()
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

    return;
}

void board_gpio_a_deinit ()
{
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4);

    return;
}
