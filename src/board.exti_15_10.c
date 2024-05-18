/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#include "board.exti_15_10.h"

#include <assert.h>

#include "stm32f4xx_hal.h"

#include "std_error/std_error.h"


#define EXTI_DEFAULT_ERROR_TEXT "EXTI_15_10 error"


static EXTI_HandleTypeDef exti_15_handler;
static EXTI_HandleTypeDef exti_12_handler;

static board_exti_15_10_config_t config;


int board_exti_15_10_init (board_exti_15_10_config_t const * const init_config, std_error_t * const error)
{
    assert(init_config != NULL);
    assert(init_config->exti_12_callback != NULL);
    assert(init_config->exti_15_callback != NULL);

    config = *init_config;

    // GPIO Ports Clock Enable
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // Configure GPIO pin
    // PB12 - T01 Reed switch
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin     = GPIO_PIN_12;
    GPIO_InitStruct.Mode    = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull    = GPIO_NOPULL;
    GPIO_InitStruct.Speed   = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    EXTI_ConfigTypeDef EXTI_InitStruct = { 0 };
    EXTI_InitStruct.Line    = EXTI_LINE_12;
    EXTI_InitStruct.Mode    = EXTI_MODE_INTERRUPT;
    EXTI_InitStruct.Trigger = EXTI_TRIGGER_RISING_FALLING;
    EXTI_InitStruct.GPIOSel = EXTI_GPIOB;

    HAL_StatusTypeDef status = HAL_EXTI_SetConfigLine(&exti_12_handler, &EXTI_InitStruct);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, EXTI_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    status = HAL_EXTI_RegisterCallback(&exti_12_handler, HAL_EXTI_COMMON_CB_ID, config.exti_12_callback);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, EXTI_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    // GPIO Ports Clock Enable
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Configure GPIO pin
    // PA15 - T01 PIR Interrupt
    GPIO_InitStruct.Pin     = GPIO_PIN_15;
    GPIO_InitStruct.Mode    = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull    = GPIO_NOPULL;
    GPIO_InitStruct.Speed   = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    EXTI_InitStruct.Line    = EXTI_LINE_15;
    EXTI_InitStruct.Mode    = EXTI_MODE_INTERRUPT;
    EXTI_InitStruct.Trigger = EXTI_TRIGGER_RISING;
    EXTI_InitStruct.GPIOSel = EXTI_GPIOA;

    status = HAL_EXTI_SetConfigLine(&exti_15_handler, &EXTI_InitStruct);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, EXTI_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    status = HAL_EXTI_RegisterCallback(&exti_15_handler, HAL_EXTI_COMMON_CB_ID, config.exti_15_callback);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, EXTI_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 7U, 0U);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    return STD_SUCCESS;
}

int board_exti_15_10_deinit (std_error_t * const error)
{
    HAL_StatusTypeDef status = HAL_EXTI_ClearConfigLine(&exti_12_handler);
    status = HAL_EXTI_ClearConfigLine(&exti_15_handler);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12);
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_15);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, EXTI_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

void EXTI15_10_IRQHandler ()
{
    HAL_EXTI_IRQHandler(&exti_12_handler);
    HAL_EXTI_IRQHandler(&exti_15_handler);

    return;
}

void board_exti_15_10_get_12 (bool * const is_high)
{
    const GPIO_PinState state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);

    if (state == GPIO_PIN_SET)
    {
        *is_high = true;
    }
    else
    {
        *is_high = false;
    }

    return;
}
