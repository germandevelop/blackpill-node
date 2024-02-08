/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#include <stdbool.h>

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

#include "board.uart_2.h"
#include "embedded_logger.h"
#include "std_error/std_error.h"


#define APPLICATION_START_ADDRESS   0x08008000  // Sector 2

#define SRAM_SIZE   (64 * 1024)             // 64 kB
#define SRAM_END    (SRAM_BASE + SRAM_SIZE)


static void board_print_uart_2 (const uint8_t *data, uint16_t data_size);
static void freeze_loop ();

int main ()
{       
    HAL_Init();

    int exit_code = board_uart_2_init(NULL);

    embedded_logger_config_t logger_config;
    logger_config.write_array_callback = board_print_uart_2;

    if (exit_code != STD_SUCCESS)
    {
        logger_config.write_array_callback = NULL;
    }

    embedded_logger_init(&logger_config);

    if (*((uint32_t*) APPLICATION_START_ADDRESS) != SRAM_END)
    {
        LOG("Bootloader: no application found\r\n");

        size_t i = 0U;

        while (true)
        {
            HAL_Delay(5U * 1000U);

            LOG("Bootloader: loop %u\r\n", i);
            ++i;
        }
    }
    else
    {
        LOG("Bootloader: jump to application\r\n");

        board_uart_2_deinit(NULL);

        SysTick->CTRL = 0x00;

        //HAL_RCC_DeInit();
        HAL_DeInit();

        RCC->CIR = 0x00000000;
        __set_MSP(*((volatile uint32_t*)APPLICATION_START_ADDRESS));
        __DMB();
        SCB->VTOR = APPLICATION_START_ADDRESS;
        __DSB();

        uint32_t jump_address = *((volatile uint32_t*)(APPLICATION_START_ADDRESS + 4));
        void (*reset_handler)(void) = (void*)jump_address;
        reset_handler();

        freeze_loop();
    }

    return 0;
}


void board_print_uart_2 (const uint8_t *data, uint16_t data_size)
{
    board_uart_2_write(data, data_size, 10U * 1000U, NULL);

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

void SysTick_Handler ()
{
    HAL_IncTick();

    return;
}