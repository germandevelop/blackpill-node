#include "stm32f4xx_it.h"

#include "stm32f4xx_hal.h"


static TIM_HandleTypeDef timer_11_handler;


static void timer_11_period_elapsed_callback (TIM_HandleTypeDef *timer_handler);

HAL_StatusTypeDef HAL_InitTick (uint32_t TickPriority)
{
    RCC_ClkInitTypeDef    clkconfig;
    uint32_t              uwTimclock = 0U;

    uint32_t              uwPrescalerValue = 0U;
    uint32_t              pFLatency;
    HAL_StatusTypeDef     status;

    // Enable TIM11 clock
    __HAL_RCC_TIM11_CLK_ENABLE();
    __HAL_RCC_TIM11_CLK_SLEEP_DISABLE();

    // Get clock configuration
    HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);

    // Compute TIM11 clock
    uwTimclock = HAL_RCC_GetPCLK2Freq();

    // Compute the prescaler value to have TIM11 counter clock equal to 1MHz
    uwPrescalerValue = (uint32_t)((uwTimclock / 1000000U) - 1U);

    // Initialize TIM11
    timer_11_handler.Instance = TIM11;

    // Initialize TIMx peripheral as follow:
    // Period = [(TIM11CLK/1000) - 1]. to have a (1/1000) s time base.
    // Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
    // ClockDivision = 0
    // Counter direction = Up
    timer_11_handler.Init.Period            = (1000000U / 1000U) - 1U;
    timer_11_handler.Init.Prescaler         = uwPrescalerValue;
    timer_11_handler.Init.ClockDivision     = 0U;
    timer_11_handler.Init.CounterMode       = TIM_COUNTERMODE_UP;
    timer_11_handler.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    status = HAL_TIM_Base_Init(&timer_11_handler);

    if (status == HAL_OK)
    {
        timer_11_handler.PeriodElapsedCallback = timer_11_period_elapsed_callback;

        // Start the TIM time Base generation in interrupt mode
        status = HAL_TIM_Base_Start_IT(&timer_11_handler);

        if (status == HAL_OK)
        {
            // Enable the TIM11 global Interrupt
            HAL_NVIC_EnableIRQ(TIM1_TRG_COM_TIM11_IRQn);

            // Configure the SysTick IRQ priority
            if (TickPriority < (1UL << __NVIC_PRIO_BITS))
            {
                // Configure the TIM IRQ priority
                HAL_NVIC_SetPriority(TIM1_TRG_COM_TIM11_IRQn, TickPriority, 0U);
                uwTickPrio = TickPriority;
            }
            else
            {
                status = HAL_ERROR;
            }
        }
    }
    return status;
}

void HAL_SuspendTick ()
{
    // Disable TIM11 update Interrupt
    __HAL_TIM_DISABLE_IT(&timer_11_handler, TIM_IT_UPDATE);

    return;
}

void HAL_ResumeTick ()
{
    // Enable TIM11 Update interrupt
    __HAL_TIM_ENABLE_IT(&timer_11_handler, TIM_IT_UPDATE);

    return;
}

void TIM1_TRG_COM_TIM11_IRQHandler ()
{
    HAL_TIM_IRQHandler(&timer_11_handler);

    return;
}

void timer_11_period_elapsed_callback (TIM_HandleTypeDef *timer_handler)
{
    UNUSED(timer_handler);

    HAL_IncTick();

    return;
}
