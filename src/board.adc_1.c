/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "board.adc_1.h"

#include "stm32f4xx_hal.h"

#include "std_error/std_error.h"


#define ADC_DEFAULT_ERROR_TEXT  "ADC_1 error"


static ADC_HandleTypeDef adc_1_handler;


static void board_adc_1_msp_init (ADC_HandleTypeDef *adc_handler);
static void board_adc_1_msp_deinit (ADC_HandleTypeDef *adc_handler);

int board_adc_1_init (std_error_t * const error)
{
    // Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
    adc_1_handler.Instance                      = ADC1;
    adc_1_handler.MspInitCallback               = board_adc_1_msp_init;
    adc_1_handler.MspDeInitCallback             = board_adc_1_msp_deinit;
    adc_1_handler.Init.ClockPrescaler           = ADC_CLOCK_SYNC_PCLK_DIV4;
    adc_1_handler.Init.Resolution               = ADC_RESOLUTION_12B;
    adc_1_handler.Init.ScanConvMode             = DISABLE;
    adc_1_handler.Init.ContinuousConvMode       = DISABLE;
    adc_1_handler.Init.DiscontinuousConvMode    = DISABLE;
    adc_1_handler.Init.ExternalTrigConvEdge     = ADC_EXTERNALTRIGCONVEDGE_NONE;
    adc_1_handler.Init.ExternalTrigConv         = ADC_SOFTWARE_START;
    adc_1_handler.Init.DataAlign                = ADC_DATAALIGN_RIGHT;
    adc_1_handler.Init.NbrOfConversion          = 1U;
    adc_1_handler.Init.DMAContinuousRequests    = DISABLE;
    adc_1_handler.Init.EOCSelection             = ADC_EOC_SINGLE_CONV;

    HAL_StatusTypeDef status = HAL_ADC_Init(&adc_1_handler);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, ADC_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    // Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
    ADC_ChannelConfTypeDef config = { 0 };
    config.Channel      = ADC_CHANNEL_9;
    config.Rank         = 1U;
    config.SamplingTime = ADC_SAMPLETIME_3CYCLES;

    status = HAL_ADC_ConfigChannel(&adc_1_handler, &config);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, ADC_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

void board_adc_1_deinit ()
{
    HAL_ADC_DeInit(&adc_1_handler);

    return;
}

void board_adc_1_enable_clock ()
{
    __HAL_RCC_ADC1_CLK_ENABLE();

    return;
}

void board_adc_1_disable_clock ()
{
    __HAL_RCC_ADC1_CLK_DISABLE();

    return;
}

int board_adc_1_read_value (uint32_t * const adc_value, uint32_t timeout_ms, std_error_t * const error)
{
    int exit_code = STD_FAILURE;

    HAL_ADC_Start(&adc_1_handler);

    const HAL_StatusTypeDef status = HAL_ADC_PollForConversion(&adc_1_handler, timeout_ms);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, ADC_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);
    }
    else
    {
        *adc_value = HAL_ADC_GetValue(&adc_1_handler);

        exit_code = STD_SUCCESS;
    }

    HAL_ADC_Stop(&adc_1_handler);

    return exit_code;
}


void board_adc_1_msp_init (ADC_HandleTypeDef *adc_handler)
{
    UNUSED(adc_handler);

    // Peripheral clock enable
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // ADC1 GPIO Configuration
    // PB1     ------> ADC1_IN9
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin     = GPIO_PIN_1;
    GPIO_InitStruct.Mode    = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull    = GPIO_NOPULL;

    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    return;
}

void board_adc_1_msp_deinit (ADC_HandleTypeDef *adc_handler)
{
    UNUSED(adc_handler);
    
    // Peripheral clock disable
    __HAL_RCC_ADC1_CLK_DISABLE();

    // ADC1 GPIO Configuration
    // PB1     ------> ADC1_IN9
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_1);

    return;
}
