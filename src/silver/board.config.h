/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// BlackPill-Silver config

// SysClock
#define CONFIG_CLOCK_REGULATOR_SCALE PWR_REGULATOR_VOLTAGE_SCALE2
#define CONFIG_CLOCK_PLLM 25U
#define CONFIG_CLOCK_PLLN 336U
#define CONFIG_CLOCK_PLLP RCC_PLLP_DIV4
#define CONFIG_CLOCK_PLLQ 7U
#define CONFIG_CLOCK_FLASH_LATENCY FLASH_LATENCY_2

// FreeRTOS
#define CONFIG_RTOS_HEAP_SIZE ((size_t)(50 * 1024)) // 50 Kbytes

#endif // BOARD_CONFIG_H