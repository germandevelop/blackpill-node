#ifndef __STM32F4XX_IT_H
#define __STM32F4XX_IT_H

void NMI_Handler ();
void HardFault_Handler ();

void MemManage_Handler ();
void BusFault_Handler ();
void UsageFault_Handler ();
void DebugMon_Handler ();

void TIM1_TRG_COM_TIM11_IRQHandler ();

#endif // __STM32F4xx_IT_H
