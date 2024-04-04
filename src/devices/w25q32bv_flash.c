/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "w25q32bv_flash.h"

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#include "std_error/std_error.h"


#define READ_JEDEC_ID           0x9F
#define READ_DATA               0x03
#define FAST_READ               0x0B
#define WRITE_ENABLE            0x06
#define PAGE_PROGRAMM           0x02
#define SECTOR_ERASE            0x20
#define BLOCK_ERASE             0xD8
#define CHIP_ERASE              0xC7
#define READ_STATUS_REGISTER_1  0x05
#define POWER_DOWN              0xB9
#define RELEASE_POWER_DOWN      0xAB

#define DUMMY_BYTE              0xA5


void w25q32bv_flash_init (w25q32bv_flash_t * const self,
                            w25q32bv_flash_config_t const * const config)
{
    assert(self                             != NULL);
    assert(config                           != NULL);
    assert(config->spi_select_callback      != NULL);
    assert(config->spi_unselect_callback    != NULL);
    assert(config->spi_tx_rx_callback       != NULL);
    assert(config->delay_callback           != NULL);

    self->config = *config;

    self->array.block_count     = 64U;
    self->array.page_size       = 256U;
    self->array.sector_size     = 4096U;
    self->array.sector_count    = self->array.block_count * 16U;
    self->array.page_count      = (self->array.sector_count * self->array.sector_size) / self->array.page_size;
    self->array.block_size      = self->array.sector_size * 16U;

    return;
}

void w25q32bv_flash_get_array (w25q32bv_flash_t const * const self,
                                w25q32bv_flash_array_t * const array)
{
    assert(self     != NULL);
    assert(array    != NULL);

    *array = self->array;

    return;
}

int w25q32bv_flash_read_info (w25q32bv_flash_t const * const self,
                                w25q32bv_flash_info_t * const info,
                                std_error_t * const error)
{
    assert(self != NULL);
    assert(info != NULL);

    info->capacity_KByte = (self->array.sector_count * self->array.sector_size) / 1024U;

    const uint16_t data_size = 4U;
    uint8_t tx_data[data_size], rx_data[data_size];

    tx_data[0] = READ_JEDEC_ID;
    tx_data[1] = DUMMY_BYTE;
    tx_data[2] = DUMMY_BYTE;
    tx_data[3] = DUMMY_BYTE;

    self->config.spi_select_callback();
    const int exit_code = self->config.spi_tx_rx_callback(tx_data, rx_data, data_size, self->config.spi_timeout_ms, error);
    self->config.spi_unselect_callback();

    info->jedec_id = (rx_data[1] << 16) | (rx_data[2] << 8) | (rx_data[3] << 0);

    return exit_code;
}

int w25q32bv_flash_read_data (  w25q32bv_flash_t const * const self,
                                uint8_t *data,
                                uint32_t size,
                                uint32_t sector_number,
                                uint32_t sector_offset,
                                std_error_t * const error)
{
    assert(self != NULL);
    assert(data != NULL);

    const uint32_t address = (sector_number * self->array.sector_size) + sector_offset;

    const uint16_t data_size = 4U;
    uint8_t tx_data[data_size], rx_data[data_size];

    tx_data[0] = READ_DATA;
    tx_data[1] = (address & 0xFF0000) >> 16;
    tx_data[2] = (address & 0xFF00) >> 8;
    tx_data[3] = address & 0xFF;

    self->config.spi_select_callback();

    int exit_code = self->config.spi_tx_rx_callback(tx_data, rx_data, data_size, self->config.spi_timeout_ms, error);

    if (exit_code != STD_FAILURE)
    {
        uint8_t dummy_data[size];

        exit_code = self->config.spi_tx_rx_callback(dummy_data, data, size, self->config.spi_timeout_ms, error);
    }
    self->config.spi_unselect_callback();

    return exit_code;
}

int w25q32bv_flash_read_data_fast (w25q32bv_flash_t const * const self,
                                    uint8_t *data,
                                    uint32_t size,
                                    uint32_t sector_number,
                                    uint32_t sector_offset,
                                    std_error_t * const error)
{
    assert(self != NULL);
    assert(data != NULL);

    const uint32_t address = (sector_number * self->array.sector_size) + sector_offset;

    const uint16_t data_size = 5U;
    uint8_t tx_data[data_size], rx_data[data_size];

    tx_data[0] = FAST_READ;
    tx_data[1] = (address & 0xFF0000) >> 16;
    tx_data[2] = (address & 0xFF00) >> 8;
    tx_data[3] = address & 0xFF;
    tx_data[4] = 0U;

    self->config.spi_select_callback();

    int exit_code = self->config.spi_tx_rx_callback(tx_data, rx_data, data_size, self->config.spi_timeout_ms, error);

    if (exit_code != STD_FAILURE)
    {
        uint8_t dummy_data[size];

        exit_code = self->config.spi_tx_rx_callback(dummy_data, data, size, self->config.spi_timeout_ms, error);
    }
    self->config.spi_unselect_callback();

    return exit_code;
}

int w25q32bv_flash_enable_erasing_or_writing (w25q32bv_flash_t const * const self, std_error_t * const error)
{
    assert(self != NULL);

    const uint16_t data_size = 1U;
    uint8_t tx_data[data_size], rx_data[data_size];

    tx_data[0] = WRITE_ENABLE;

    const uint32_t delay_ms = 1U;

    self->config.spi_select_callback();
    const int exit_code = self->config.spi_tx_rx_callback(tx_data, rx_data, data_size, self->config.spi_timeout_ms, error);
    self->config.delay_callback(delay_ms);
    self->config.spi_unselect_callback();

    return exit_code;
}

int w25q32bv_flash_erase_sector (w25q32bv_flash_t const * const self, uint32_t sector_number, std_error_t * const error)
{
    assert(self != NULL);

    const uint32_t address = sector_number * self->array.sector_size;

    const uint16_t data_size = 4U;
    uint8_t tx_data[data_size], rx_data[data_size];

    tx_data[0] = SECTOR_ERASE;
    tx_data[1] = (address & 0xFF0000) >> 16;
    tx_data[2] = (address & 0xFF00) >> 8;
    tx_data[3] = address & 0xFF;

    self->config.spi_select_callback();
    const int exit_code = self->config.spi_tx_rx_callback(tx_data, rx_data, data_size, self->config.spi_timeout_ms, error);
    self->config.spi_unselect_callback();

    return exit_code;
}

int w25q32bv_flash_erase_block (w25q32bv_flash_t const * const self, uint32_t block_number, std_error_t * const error)
{
    assert(self != NULL);

    const uint32_t address = block_number * self->array.block_size;

    const uint16_t data_size = 4U;
    uint8_t tx_data[data_size], rx_data[data_size];

    tx_data[0] = BLOCK_ERASE;
    tx_data[1] = (address & 0xFF0000) >> 16;
    tx_data[2] = (address & 0xFF00) >> 8;
    tx_data[3] = address & 0xFF;

    self->config.spi_select_callback();
    const int exit_code = self->config.spi_tx_rx_callback(tx_data, rx_data, data_size, self->config.spi_timeout_ms, error);
    self->config.spi_unselect_callback();

    return exit_code;
}

int w25q32bv_flash_erase_chip (w25q32bv_flash_t const * const self, std_error_t * const error)
{
    assert(self != NULL);

    const uint16_t data_size = 1U;
    uint8_t tx_data[data_size], rx_data[data_size];

    tx_data[0] = CHIP_ERASE;

    self->config.spi_select_callback();
    const int exit_code = self->config.spi_tx_rx_callback(tx_data, rx_data, data_size, self->config.spi_timeout_ms, error);
    self->config.spi_unselect_callback();

    return exit_code;
}

int w25q32bv_flash_write_page (w25q32bv_flash_t const * const self,
                                uint8_t *data,
                                uint32_t size,
                                uint32_t page_number,
                                uint32_t page_offset,
                                std_error_t * const error)
{
    assert(self != NULL);
    assert(data != NULL);

    if (((size + page_offset) > self->array.page_size) || (size == 0U))
    {
        std_error_catch_invalid_argument(error, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    const uint32_t address = (page_number * self->array.page_size) + page_offset;

    const uint16_t data_size = 4U;
    uint8_t tx_data[data_size], rx_data[data_size];

    tx_data[0] = PAGE_PROGRAMM;
    tx_data[1] = (address & 0xFF0000) >> 16;
    tx_data[2] = (address & 0xFF00) >> 8;
    tx_data[3] = address & 0xFF;

    self->config.spi_select_callback();

    int exit_code = self->config.spi_tx_rx_callback(tx_data, rx_data, data_size, self->config.spi_timeout_ms, error);

    if (exit_code != STD_FAILURE)
    {
        uint8_t dummy_data[size];

        exit_code = self->config.spi_tx_rx_callback(data, dummy_data, size, self->config.spi_timeout_ms, error);
    }
    self->config.spi_unselect_callback();

    return exit_code;
}

int w25q32bv_flash_wait_erasing_or_writing (w25q32bv_flash_t const * const self, std_error_t * const error)
{
    assert(self != NULL);

    const uint16_t data_size = 1U;
    uint8_t tx_data[data_size], rx_data[data_size];

    tx_data[0] = READ_STATUS_REGISTER_1;

    self->config.spi_select_callback();

    int exit_code = self->config.spi_tx_rx_callback(tx_data, rx_data, data_size, self->config.spi_timeout_ms, error);

    if (exit_code != STD_FAILURE)
    {
        const uint32_t delay_ms = 1U;
        
        tx_data[0] = DUMMY_BYTE;

        while (true)
        {
            exit_code = self->config.spi_tx_rx_callback(tx_data, rx_data, data_size, self->config.spi_timeout_ms, error);

            if (exit_code != STD_SUCCESS)
            {
                break;
            }

            if ((rx_data[0] & 1U) == 0U)
            {
                break;
            }

            self->config.delay_callback(delay_ms);
        }
    }
    self->config.spi_unselect_callback();
    
    return exit_code;
}

int w25q32bv_flash_power_down (w25q32bv_flash_t const * const self, std_error_t * const error)
{
    assert(self != NULL);

    const uint16_t data_size = 1U;
    uint8_t tx_data[data_size], rx_data[data_size];

    tx_data[0] = POWER_DOWN;

    self->config.spi_select_callback();
    const int exit_code = self->config.spi_tx_rx_callback(tx_data, rx_data, data_size, self->config.spi_timeout_ms, error);
    self->config.spi_unselect_callback();

    return exit_code;
}

int w25q32bv_flash_release_power_down (w25q32bv_flash_t const * const self, std_error_t * const error)
{
    assert(self != NULL);

    const uint16_t data_size = 1U;
    uint8_t tx_data[data_size], rx_data[data_size];

    tx_data[0] = RELEASE_POWER_DOWN;

    self->config.spi_select_callback();
    const int exit_code = self->config.spi_tx_rx_callback(tx_data, rx_data, data_size, self->config.spi_timeout_ms, error);
    self->config.spi_unselect_callback();

    return exit_code;
}
