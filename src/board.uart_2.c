/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "board.uart_2.h"

#include "stm32f4xx_hal.h"

#include "std_error/std_error.h"


#define UART_DEFAULT_ERROR_TEXT "UART_2 error"


static UART_HandleTypeDef uart_2_handler;


static void board_uart_2_msp_init (UART_HandleTypeDef *uart_handler);
static void board_uart_2_msp_deinit (UART_HandleTypeDef *uart_handler);

int board_uart_2_init (std_error_t * const error)
{
    // UART2 parameter configuration
    uart_2_handler.Instance             = USART2;
    uart_2_handler.MspInitCallback      = board_uart_2_msp_init;
    uart_2_handler.MspDeInitCallback    = board_uart_2_msp_deinit;
    uart_2_handler.Init.BaudRate        = 115200U;
    uart_2_handler.Init.WordLength      = UART_WORDLENGTH_8B;
    uart_2_handler.Init.StopBits        = UART_STOPBITS_1;
    uart_2_handler.Init.Parity          = UART_PARITY_NONE;
    uart_2_handler.Init.Mode            = UART_MODE_TX;
    uart_2_handler.Init.HwFlowCtl       = UART_HWCONTROL_NONE;

    const HAL_StatusTypeDef status = HAL_UART_Init(&uart_2_handler);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, UART_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

int board_uart_2_deinit (std_error_t * const error)
{
    const HAL_StatusTypeDef status = HAL_UART_DeInit(&uart_2_handler);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, UART_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

int board_uart_2_write (const uint8_t *data,
                        uint16_t data_size,
                        uint32_t timeout_ms,
                        std_error_t * const error)
{
    const HAL_StatusTypeDef status = HAL_UART_Transmit(&uart_2_handler, data, data_size, timeout_ms);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, UART_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}


void board_uart_2_msp_init (UART_HandleTypeDef *uart_handler)
{
    UNUSED(uart_handler);
    
    // Peripheral clock enable
    __HAL_RCC_USART2_CLK_SLEEP_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // USART2 GPIO Configuration
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin         = GPIO_PIN_2;
    GPIO_InitStruct.Mode        = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull        = GPIO_PULLUP;
    GPIO_InitStruct.Speed       = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate   = GPIO_AF7_USART2;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    return;
}

void board_uart_2_msp_deinit (UART_HandleTypeDef *uart_handler)
{
    UNUSED(uart_handler);
    
    // Peripheral clock disable
    __HAL_RCC_USART2_CLK_DISABLE();

    // USART2 GPIO Configuration
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2);

    return;
}
