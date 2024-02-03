#include "stm32f4xx_hal.h"


void HAL_MspInit ()
{
    // ???
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

#ifndef NDEBUG
    // Init system interrupt
    HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0U, 0U);
    HAL_NVIC_SetPriority(BusFault_IRQn, 0U, 0U);
    HAL_NVIC_SetPriority(UsageFault_IRQn, 0U, 0U);

    // HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0U, 0U);
    // SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;   // Enable UsageFault for 'Divide by zero'
    // SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk; // Enable UsageFault for 'Unaligned memory'

    SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk; // Enable MemManage IRQ
    SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk; // Enable BusFault IRQ
    SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk; // Enable UsageFault IRQ
#endif // NDEBUG

    // For FreeRTOS
    HAL_NVIC_SetPriority(SVCall_IRQn, 0U, 0U);
    HAL_NVIC_SetPriority(PendSV_IRQn, 15U, 0U);
    HAL_NVIC_SetPriority(SysTick_IRQn, 15U, 0U);

    return;
}
