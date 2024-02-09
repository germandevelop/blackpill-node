/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include <stdbool.h>

#include "stm32f4xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"

#include "board.h"
#include "board.config.h"


static IWDG_HandleTypeDef iwdg_handler;


static void system_clock_init ();
static void watchdog_init ();
static void watchdog_refresh ();
static void freeze_loop ();

int main ()
{       
    HAL_Init();
    system_clock_init();
    watchdog_init();

    board_config_t board_config;
    board_config.refresh_watchdog_callback  = watchdog_refresh;
    board_config.watchdog_timeout_ms        = 25U * 1000U;

    if (board_init(&board_config, NULL) != SUCCESS)
    {
        freeze_loop();
    }

    vTaskStartScheduler();

    return 0;
}


void system_clock_init ()
{
    // Configure the main internal regulator output voltage
    // ???
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(CONFIG_CLOCK_REGULATOR_SCALE);

    // Initializes the RCC Oscillators according to the specified parameters
    // in the RCC_OscInitTypeDef structure.
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_OscInitStruct.OscillatorType    = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.LSIState          = RCC_LSI_ON;
    RCC_OscInitStruct.HSEState          = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState      = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource     = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM          = CONFIG_CLOCK_PLLM;
    RCC_OscInitStruct.PLL.PLLN          = CONFIG_CLOCK_PLLN;
    RCC_OscInitStruct.PLL.PLLP          = CONFIG_CLOCK_PLLP;
    RCC_OscInitStruct.PLL.PLLQ          = CONFIG_CLOCK_PLLQ;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        freeze_loop();
    }

    // Initializes the CPU, AHB and APB buses clocks
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
    RCC_ClkInitStruct.ClockType         = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource      = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider     = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider    = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider    = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, CONFIG_CLOCK_FLASH_LATENCY) != HAL_OK)
    {
        freeze_loop();
    }

    // Disable HSI clock
    __HAL_RCC_HSI_DISABLE();

    return;
}

void watchdog_init ()
{
    iwdg_handler.Instance       = IWDG;
    iwdg_handler.Init.Prescaler = IWDG_PRESCALER_256;
    iwdg_handler.Init.Reload    = 4095U;

    if (HAL_IWDG_Init(&iwdg_handler) != HAL_OK)
    {
        freeze_loop();
    }
    return;
}

void watchdog_refresh ()
{
    HAL_IWDG_Refresh(&iwdg_handler);

    return;
}

void freeze_loop ()
{
    __disable_irq();

    while (true)
    {
    }
    return;
}
