/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_flash_ex.h"

#include "storage.h"

#include "board.config.h"
#include "board.uart_2.h"
#include "board.spi_1.h"
#include "board.gpio_a.h"

#include "logger.h"
#include "std_error/std_error.h"


#define APPLICATION_START_ADDRESS   0x08010000  // Sector 4
#define SRAM_END    (SRAM_BASE + SRAM_SIZE)

#define UART_TIMEOUT_MS (1U * 1000U)    // 1 sec
#define SPI_TIMEOUT_MS  (1U * 1000U)    // 1 sec

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


static void bootloader_loop ();
static void board_print_uart_2 (const uint8_t *data, uint16_t data_size);
static void spi_1_lock ();
static void freeze_loop ();

int main ()
{
    HAL_Init();

    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000U);
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

    int exit_code = board_uart_2_init(NULL);

    logger_config_t logger_config;
    logger_config.write_array_callback = board_print_uart_2;

    if (exit_code != STD_SUCCESS)
    {
        logger_config.write_array_callback = NULL;
    }

    logger_init(&logger_config);

    std_error_t error;
    std_error_init(&error);

    LOG("Bootloader [GPIO_A] : init\r\n");

    board_gpio_a_init();

    LOG("Bootloader [SPI_1] : init\r\n");

    if (board_spi_1_init(&error) != STD_SUCCESS)
    {
        LOG("Bootloader [SPI_1] : %s\r\n", error.text);

        bootloader_loop();
    }

    LOG("Bootloader [storage] : init\r\n");

    storage_config_t config;
    config.spi_lock_callback        = spi_1_lock;
    config.spi_unlock_callback      = spi_1_lock;
    config.spi_select_callback      = board_gpio_a_pin_4_reset;
    config.spi_unselect_callback    = board_gpio_a_pin_4_set;
    config.spi_tx_rx_callback       = board_spi_1_read_write;
    config.spi_timeout_ms           = SPI_TIMEOUT_MS;
    config.delay_callback           = HAL_Delay;

    storage_t storage;

    if (storage_init(&storage, &config, &error) != STD_SUCCESS)
    {
        LOG("Bootloader [storage] : %s\r\n", error.text);

        bootloader_loop();
    }

    if (storage_enable_power(&storage, &error) != STD_SUCCESS)
    {
        LOG("Bootloader [storage] : %s\r\n", error.text);

        bootloader_loop();
    }

    if (storage_mount_filesystem(&storage, &error) != STD_SUCCESS)
    {
        LOG("Bootloader [storage] : %s\r\n", error.text);

        bootloader_loop();
    }

    const char firmware_file_name[64] = "firmware\0";
    storage_file_t firmware_file;

    if (storage_open_file(&storage, &firmware_file, firmware_file_name, &error) == STD_SUCCESS)
    {
        HAL_FLASH_Unlock();

        LOG("Bootloader [flash] : earse firmware\r\n");
        
        FLASH_EraseInitTypeDef erase_init;
        erase_init.Sector       = FLASH_SECTOR_4;
        erase_init.NbSectors    = 2;
        erase_init.TypeErase    = FLASH_TYPEERASE_SECTORS;
        erase_init.VoltageRange = VOLTAGE_RANGE_3;

        uint32_t sector_error;

        if (HAL_FLASHEx_Erase(&erase_init, &sector_error) != HAL_OK)
        {
            LOG("Bootloader [flash] : earse sector error = %lu\r\n", sector_error);
        }

        char firmware_data[LFS_CACHE_SIZE];

        uint32_t flash_address = APPLICATION_START_ADDRESS;

        while (true)
        {
            size_t size;

            if (storage_read_file(&storage, &firmware_file, firmware_data, &size, ARRAY_SIZE(firmware_data), &error) != STD_SUCCESS)
            {
                LOG("Bootloader [storage] : %s\r\n", error.text);

                bootloader_loop();
            }

            if (size == 0U)
            {
                break;
            }

            LOG("Bootloader [flash] : program bytes = %u\r\n", size);

            for (size_t i = 0U; i < size; ++i)
            {
                HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, flash_address, (uint64_t)firmware_data[i]);

                ++flash_address;
            }
        }

        HAL_FLASH_Lock();

        if (storage_close_file(&storage, &firmware_file, &error) != STD_SUCCESS)
        {
            LOG("Bootloader [storage] : %s\r\n", error.text);

            bootloader_loop();
        }

        if (storage_remove_file(&storage, firmware_file_name, &error) != STD_SUCCESS)
        {
            LOG("Bootloader [storage] : %s\r\n", error.text);

            bootloader_loop();
        }
    }

    if (storage_unmount_filesystem(&storage, &error) != STD_SUCCESS)
    {
        LOG("Bootloader [storage] : %s\r\n", error.text);

        bootloader_loop();
    }

    if (storage_disable_power(&storage, &error) != STD_SUCCESS)
    {
        LOG("Bootloader [storage] : %s\r\n", error.text);

        bootloader_loop();
    }


    // Jump to the application
    if (*((uint32_t*) APPLICATION_START_ADDRESS) != SRAM_END)
    {
        LOG("Bootloader : no application found\r\n");

        bootloader_loop();
    }
    else
    {
        LOG("Bootloader : jump to application\r\n");

        board_uart_2_deinit(NULL);

        SysTick->CTRL = 0x00;

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


void bootloader_loop ()
{
    size_t i = 0U;

    while (true)
    {
        HAL_Delay(5U * 1000U);

        LOG("Bootloader : loop %u\r\n", i);
        ++i;
    }

    return;
}

void board_print_uart_2 (const uint8_t *data, uint16_t data_size)
{
    board_uart_2_write(data, data_size, UART_TIMEOUT_MS, NULL);

    return;
}

void spi_1_lock ()
{
    // Do nothing

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
