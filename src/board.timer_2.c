/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "board.timer_2.h"

#include <assert.h>

#include "stm32f4xx_hal.h"

#include "std_error/std_error.h"


#define TIMER_DEFAULT_ERROR_TEXT "TIMER_2 error"


static TIM_HandleTypeDef timer_2_handler;

static board_timer_2_config_t config;


static void board_timer_2_ic_msp_init (TIM_HandleTypeDef *timer_handler);
static void board_timer_2_ic_msp_deinit (TIM_HandleTypeDef *timer_handler);

static void board_timer_2_ic_capture_callback (TIM_HandleTypeDef *timer_handler);

static void board_timer_2_pwm_msp_init (TIM_HandleTypeDef *timer_handler);
static void board_timer_2_pwm_msp_deinit (TIM_HandleTypeDef *timer_handler);
static void board_timer_2_pwm_msp_post_init ();

int board_timer_2_init (board_timer_2_config_t const * const init_config, std_error_t * const error)
{
    assert(init_config != NULL);
    assert(init_config->ic_isr_callback != NULL);

    config = *init_config;

    timer_2_handler.Instance                = TIM2;
    timer_2_handler.IC_MspInitCallback      = board_timer_2_ic_msp_init;
    timer_2_handler.IC_MspDeInitCallback    = board_timer_2_ic_msp_deinit;
    timer_2_handler.PWM_MspInitCallback     = board_timer_2_pwm_msp_init;
    timer_2_handler.PWM_MspDeInitCallback   = board_timer_2_pwm_msp_deinit;
    timer_2_handler.Init.Prescaler          = 1000U;
    timer_2_handler.Init.CounterMode        = TIM_COUNTERMODE_UP;
    timer_2_handler.Init.Period             = 420000U;
    timer_2_handler.Init.ClockDivision      = TIM_CLOCKDIVISION_DIV1;
    timer_2_handler.Init.AutoReloadPreload  = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_StatusTypeDef status = HAL_TIM_IC_Init(&timer_2_handler);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, TIMER_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    status = HAL_TIM_PWM_Init(&timer_2_handler);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, TIMER_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    TIM_MasterConfigTypeDef master_config = { 0 };
    master_config.MasterOutputTrigger   = TIM_TRGO_RESET;
    master_config.MasterSlaveMode       = TIM_MASTERSLAVEMODE_DISABLE;

    status = HAL_TIMEx_MasterConfigSynchronization(&timer_2_handler, &master_config);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, TIMER_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    TIM_IC_InitTypeDef IC_config = { 0 };
    IC_config.ICPolarity    = TIM_INPUTCHANNELPOLARITY_FALLING;
    IC_config.ICSelection   = TIM_ICSELECTION_DIRECTTI;
    IC_config.ICPrescaler   = TIM_ICPSC_DIV1;
    IC_config.ICFilter      = 2U;

    status = HAL_TIM_IC_ConfigChannel(&timer_2_handler, &IC_config, TIM_CHANNEL_3);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, TIMER_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    TIM_OC_InitTypeDef OC_config = { 0 };
    OC_config.OCMode        = TIM_OCMODE_PWM1;
    OC_config.Pulse         = 420000U - 20000U;
    OC_config.OCPolarity    = TIM_OCPOLARITY_LOW;
    OC_config.OCFastMode    = TIM_OCFAST_DISABLE;

    status = HAL_TIM_PWM_ConfigChannel(&timer_2_handler, &OC_config, TIM_CHANNEL_2);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, TIMER_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    board_timer_2_pwm_msp_post_init();

    timer_2_handler.IC_CaptureCallback = board_timer_2_ic_capture_callback;
    
    status = HAL_TIM_IC_Start_IT(&timer_2_handler, TIM_CHANNEL_3);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, TIMER_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

void board_timer_2_deinit ()
{
    HAL_TIM_PWM_DeInit(&timer_2_handler);
    HAL_TIM_IC_DeInit(&timer_2_handler);

    return;
}

int board_timer_2_start_channel_2 (std_error_t * const error)
{
    const HAL_StatusTypeDef status = HAL_TIM_PWM_Start(&timer_2_handler, TIM_CHANNEL_2);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, TIMER_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

void board_timer_2_stop_channel_2 ()
{
    HAL_TIM_PWM_Stop(&timer_2_handler, TIM_CHANNEL_2);

    return;
}

void TIM2_IRQHandler ()
{
    HAL_TIM_IRQHandler(&timer_2_handler);

    return;
}


void board_timer_2_ic_msp_init (TIM_HandleTypeDef *timer_handler)
{
    UNUSED(timer_handler);

    // Peripheral clock enable
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // TIM2 GPIO Configuration
    // PB10     ------> TIM2_CH3
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin         = GPIO_PIN_10;
    GPIO_InitStruct.Mode        = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull        = GPIO_NOPULL;
    GPIO_InitStruct.Speed       = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate   = GPIO_AF1_TIM2;

    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // TIM2 interrupt Init
    HAL_NVIC_SetPriority(TIM2_IRQn, 7U, 0U);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    return;
}

void board_timer_2_ic_msp_deinit (TIM_HandleTypeDef *timer_handler)
{
    UNUSED(timer_handler);
    
    // Peripheral clock disable 
    __HAL_RCC_TIM2_CLK_DISABLE();

    // TIM2 GPIO Configuration
    // PB10     ------> TIM2_CH3
    // PB3     ------> TIM2_CH2
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);

    // TIM2 interrupt DeInit
    HAL_NVIC_DisableIRQ(TIM2_IRQn);

    return;
}

void board_timer_2_ic_capture_callback (TIM_HandleTypeDef *timer_handler)
{
    UNUSED(timer_handler);

    const uint32_t captured_value = HAL_TIM_ReadCapturedValue(&timer_2_handler, TIM_CHANNEL_3);

    __HAL_TIM_SET_COUNTER(&timer_2_handler, 0U);

    config.ic_isr_callback(captured_value);

    return;
}

void board_timer_2_pwm_msp_init (TIM_HandleTypeDef *timer_handler)
{
    UNUSED(timer_handler);

    return;
}

void board_timer_2_pwm_msp_deinit (TIM_HandleTypeDef *timer_handler)
{
    UNUSED(timer_handler);
    
    return;
}

void board_timer_2_pwm_msp_post_init ()
{
    // TIM2 GPIO Configuration
    // PB3     ------> TIM2_CH2
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin         = GPIO_PIN_3;
    GPIO_InitStruct.Mode        = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull        = GPIO_NOPULL;
    GPIO_InitStruct.Speed       = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate   = GPIO_AF1_TIM2;

    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    return;
}
