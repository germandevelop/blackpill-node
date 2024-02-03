#include "stm32f4xx_it.h"

#include <stdbool.h>


// Cortex-M4 Processor Interruption and Exception Handlers

// This function handles Non maskable interrupt.
void NMI_Handler ()
{
    while (true)
    {
    }
    return;
}

// This function handles Hard fault interrupt.
void HardFault_Handler ()
{
    while (true)
    {
    }
    return;
}

// This function handles Memory management fault.
void MemManage_Handler ()
{
    while (true)
    {
    }
    return;
}

// This function handles Pre-fetch fault, memory access fault.
void BusFault_Handler ()
{
    while (true)
    {
    }
    return;
}

// This function handles Undefined instruction or illegal state.
void UsageFault_Handler ()
{
    while (true)
    {
    }
    return;
}

// This function handles Debug monitor.
void DebugMon_Handler ()
{
    return;
}
