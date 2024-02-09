/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// BlackPill-Gold config

#define SRAM_SIZE   (128 * 1024)    // 128 kB

// SysClock
#define CONFIG_CLOCK_REGULATOR_SCALE PWR_REGULATOR_VOLTAGE_SCALE1
#define CONFIG_CLOCK_PLLM 25U
#define CONFIG_CLOCK_PLLN 192U
#define CONFIG_CLOCK_PLLP RCC_PLLP_DIV2
#define CONFIG_CLOCK_PLLQ 4U
#define CONFIG_CLOCK_FLASH_LATENCY FLASH_LATENCY_3

// FreeRTOS
#define CONFIG_RTOS_HEAP_SIZE ((size_t)(110 * 1024)) // 110 Kbytes

#endif // BOARD_CONFIG_H