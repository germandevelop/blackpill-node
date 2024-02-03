/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "board.timer_3.h"

#include "stm32f4xx_hal.h"

#include "std_error/std_error.h"


#define TIMER_DEFAULT_ERROR_TEXT "TIMER_3 error"


static TIM_HandleTypeDef timer_3_handler;

static void board_timer_3_pwm_msp_init (TIM_HandleTypeDef *timer_handler);
static void board_timer_3_pwm_msp_deinit (TIM_HandleTypeDef *timer_handler);
static void board_timer_3_pwm_msp_post_init ();

int board_timer_3_init (std_error_t * const error)
{
    timer_3_handler.Instance                = TIM3;
    timer_3_handler.PWM_MspInitCallback     = board_timer_3_pwm_msp_init;
    timer_3_handler.PWM_MspDeInitCallback   = board_timer_3_pwm_msp_deinit;
    timer_3_handler.Init.Prescaler          = 10000U;
    timer_3_handler.Init.CounterMode        = TIM_COUNTERMODE_UP;
    timer_3_handler.Init.Period             = 42000U;
    timer_3_handler.Init.ClockDivision      = TIM_CLOCKDIVISION_DIV1;
    timer_3_handler.Init.AutoReloadPreload  = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_StatusTypeDef status = HAL_TIM_PWM_Init(&timer_3_handler);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, TIMER_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    TIM_MasterConfigTypeDef master_config = { 0 };
    master_config.MasterOutputTrigger   = TIM_TRGO_RESET;
    master_config.MasterSlaveMode       = TIM_MASTERSLAVEMODE_DISABLE;

    status = HAL_TIMEx_MasterConfigSynchronization(&timer_3_handler, &master_config);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, TIMER_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    TIM_OC_InitTypeDef OC_config = { 0 };
    OC_config.OCMode        = TIM_OCMODE_PWM1;
    OC_config.Pulse         = 42000U - 2000U;
    OC_config.OCPolarity    = TIM_OCPOLARITY_LOW;
    OC_config.OCFastMode    = TIM_OCFAST_DISABLE;

    status = HAL_TIM_PWM_ConfigChannel(&timer_3_handler, &OC_config, TIM_CHANNEL_1);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, TIMER_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    status = HAL_TIM_PWM_ConfigChannel(&timer_3_handler, &OC_config, TIM_CHANNEL_2);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, TIMER_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    board_timer_3_pwm_msp_post_init();

    return STD_SUCCESS;
}

void board_timer_3_deinit ()
{
    HAL_TIM_PWM_DeInit(&timer_3_handler);

    return;
}

void board_timer_3_enable_clock ()
{
    __HAL_RCC_TIM3_CLK_ENABLE();

    return;
}

void board_timer_3_disable_clock ()
{
    __HAL_RCC_TIM3_CLK_DISABLE();

    return;
}

int board_timer_3_start_channel_1 (std_error_t * const error)
{
    const HAL_StatusTypeDef status = HAL_TIM_PWM_Start(&timer_3_handler, TIM_CHANNEL_1);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, TIMER_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

int board_timer_3_start_channel_2 (std_error_t * const error)
{
    const HAL_StatusTypeDef status = HAL_TIM_PWM_Start(&timer_3_handler, TIM_CHANNEL_2);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, TIMER_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}


void board_timer_3_pwm_msp_init (TIM_HandleTypeDef *timer_handler)
{
    UNUSED(timer_handler);

    // Peripheral clock enable
    __HAL_RCC_TIM3_CLK_ENABLE();

    return;
}

void board_timer_3_pwm_msp_deinit (TIM_HandleTypeDef *timer_handler)
{
    UNUSED(timer_handler);

    // Peripheral clock disable
    __HAL_RCC_TIM3_CLK_DISABLE();

    return;
}

void board_timer_3_pwm_msp_post_init ()
{
    // Peripheral clock enable
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // TIM3 GPIO Configuration
    // PB4     ------> TIM3_CH1
    // PB5     ------> TIM3_CH2
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin         = GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode        = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull        = GPIO_NOPULL;
    GPIO_InitStruct.Speed       = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate   = GPIO_AF2_TIM3;

    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    return;
}
