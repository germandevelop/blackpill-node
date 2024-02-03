/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "board.exti_2.h"

#include <assert.h>

#include "stm32f4xx_hal.h"

#include "std_error/std_error.h"


#define EXTI_DEFAULT_ERROR_TEXT "EXTI_2 error"


static EXTI_HandleTypeDef exti_2_handler;


int board_exti_2_init (board_exti_2_callback_t exti_2_callback, std_error_t * const error)
{
    assert(exti_2_callback != NULL);

    // GPIO Ports Clock Enable
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // Configure GPIO pin
    // PB2 - MCP23017 Expander Interrupt
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin     = GPIO_PIN_2;
    GPIO_InitStruct.Mode    = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull    = GPIO_NOPULL;
    GPIO_InitStruct.Speed   = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    EXTI_ConfigTypeDef EXTI_InitStruct = { 0 };
    EXTI_InitStruct.Line    = EXTI_LINE_2;
    EXTI_InitStruct.Mode    = EXTI_MODE_INTERRUPT;
    EXTI_InitStruct.Trigger = EXTI_TRIGGER_RISING;
    EXTI_InitStruct.GPIOSel = EXTI_GPIOB;

    HAL_StatusTypeDef status = HAL_EXTI_SetConfigLine(&exti_2_handler, &EXTI_InitStruct);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, EXTI_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    status = HAL_EXTI_RegisterCallback(&exti_2_handler, HAL_EXTI_COMMON_CB_ID, exti_2_callback);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, EXTI_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    HAL_NVIC_SetPriority(EXTI2_IRQn, 7U, 0U);
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);

    return STD_SUCCESS;
}

int board_exti_2_deinit (std_error_t * const error)
{
    const HAL_StatusTypeDef status = HAL_EXTI_ClearConfigLine(&exti_2_handler);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_2);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, EXTI_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

void EXTI2_IRQHandler ()
{
    HAL_EXTI_IRQHandler(&exti_2_handler);

    return;
}
