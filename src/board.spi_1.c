/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "board.spi_1.h"

#include "stm32f4xx_hal.h"

#include "std_error/std_error.h"


#define SPI_DEFAULT_ERROR_TEXT  "SPI_1 error"


static SPI_HandleTypeDef spi_1_handler;


static void board_spi_1_msp_init (SPI_HandleTypeDef *spi_handler);
static void board_spi_1_msp_deinit (SPI_HandleTypeDef *spi_handler);

int board_spi_1_init (std_error_t * const error)
{
    // SPI1 parameter configuration
    spi_1_handler.Instance                  = SPI1;
    spi_1_handler.MspInitCallback           = board_spi_1_msp_init;
    spi_1_handler.MspDeInitCallback         = board_spi_1_msp_deinit;
    spi_1_handler.Init.Mode                 = SPI_MODE_MASTER;
    spi_1_handler.Init.Direction            = SPI_DIRECTION_2LINES;
    spi_1_handler.Init.DataSize             = SPI_DATASIZE_8BIT;
    spi_1_handler.Init.CLKPolarity          = SPI_POLARITY_LOW;
    spi_1_handler.Init.CLKPhase             = SPI_PHASE_1EDGE;
    spi_1_handler.Init.NSS                  = SPI_NSS_SOFT;
    spi_1_handler.Init.BaudRatePrescaler    = SPI_BAUDRATEPRESCALER_2;
    spi_1_handler.Init.FirstBit             = SPI_FIRSTBIT_MSB;
    spi_1_handler.Init.TIMode               = SPI_TIMODE_DISABLE;
    spi_1_handler.Init.CRCCalculation       = SPI_CRCCALCULATION_DISABLE;
    spi_1_handler.Init.CRCPolynomial        = 10U;

    const HAL_StatusTypeDef status = HAL_SPI_Init(&spi_1_handler);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, SPI_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

void board_spi_1_deinit ()
{
    HAL_SPI_DeInit(&spi_1_handler);

    return;
}

void board_spi_1_enable_clock ()
{
    __HAL_RCC_SPI1_CLK_ENABLE();

    return;
}

void board_spi_1_disable_clock ()
{
    __HAL_RCC_SPI1_CLK_DISABLE();

    return;
}

int board_spi_1_read (uint8_t *data,
                        uint16_t size,
                        uint32_t timeout_ms,
                        std_error_t * const error)
{
    const HAL_StatusTypeDef status = HAL_SPI_Receive(&spi_1_handler, data, size, timeout_ms);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, SPI_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

int board_spi_1_write (uint8_t *data,
                        uint16_t size,
                        uint32_t timeout_ms,
                        std_error_t * const error)
{
    const HAL_StatusTypeDef status = HAL_SPI_Transmit(&spi_1_handler, data, size, timeout_ms);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, SPI_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

int board_spi_1_read_write (uint8_t *tx_data,
                            uint8_t *rx_data,
                            uint16_t size,
                            uint32_t timeout_ms,
                            std_error_t * const error)
{
    const HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&spi_1_handler, tx_data, rx_data, size, timeout_ms);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, SPI_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}


void board_spi_1_msp_init (SPI_HandleTypeDef *spi_handler)
{
    UNUSED(spi_handler);

    // Peripheral clock enable
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // SPI1 GPIO Configuration
    // PA5     ------> SPI1_SCK
    // PA6     ------> SPI1_MISO
    // PA7     ------> SPI1_MOSI
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin         = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode        = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull        = GPIO_NOPULL;
    GPIO_InitStruct.Speed       = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate   = GPIO_AF5_SPI1;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // SPI1 interrupt Init
    //HAL_NVIC_SetPriority(SPI1_IRQn, 5U, 0U);
    //HAL_NVIC_EnableIRQ(SPI1_IRQn);

    return;
}

void board_spi_1_msp_deinit (SPI_HandleTypeDef *spi_handler)
{
    UNUSED(spi_handler);
    
    // Peripheral clock disable
    __HAL_RCC_SPI1_CLK_DISABLE();

    // SPI1 GPIO Configuration
    // PA5     ------> SPI1_SCK
    // PA6     ------> SPI1_MISO
    // PA7     ------> SPI1_MOSI
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);

    // SPI1 interrupt DeInit
    //HAL_NVIC_DisableIRQ(SPI1_IRQn);

    return;
}
