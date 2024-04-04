/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef W25Q32BV_FLASH_H
#define W25Q32BV_FLASH_H

#include <stdint.h>

typedef struct std_error std_error_t;
typedef void (*w25q32bv_flash_spi_select_callback_t) ();
typedef int (*w25q32bv_flash_spi_tx_rx_callback_t) (uint8_t *tx_data, uint8_t *rx_data, uint16_t size,
                                                    uint32_t timeout_ms, std_error_t * const error);
typedef void (*w25q32bv_flash_delay_callback_t) (uint32_t delay_ms);

typedef struct w25q32bv_flash_config
{
    w25q32bv_flash_spi_select_callback_t spi_select_callback;
    w25q32bv_flash_spi_select_callback_t spi_unselect_callback;
    w25q32bv_flash_spi_tx_rx_callback_t spi_tx_rx_callback;
    uint32_t spi_timeout_ms;

    w25q32bv_flash_delay_callback_t delay_callback;

} w25q32bv_flash_config_t;

typedef struct w25q32bv_flash_info
{
    uint32_t jedec_id;
    uint32_t capacity_KByte;

} w25q32bv_flash_info_t;

typedef struct w25q32bv_flash_array
{
    uint32_t page_size;
    uint32_t page_count;
    uint32_t sector_size;
    uint32_t sector_count;
    uint32_t block_size;
    uint32_t block_count;

} w25q32bv_flash_array_t;

typedef struct w25q32bv_flash w25q32bv_flash_t;

void w25q32bv_flash_init (  w25q32bv_flash_t * const self,
                            w25q32bv_flash_config_t const * const config);

void w25q32bv_flash_get_array ( w25q32bv_flash_t const * const self,
                                w25q32bv_flash_array_t * const array);

int w25q32bv_flash_read_info (  w25q32bv_flash_t const * const self,
                                w25q32bv_flash_info_t * const info,
                                std_error_t * const error);

int w25q32bv_flash_read_data (  w25q32bv_flash_t const * const self,
                                uint8_t *data,
                                uint32_t size,
                                uint32_t sector_number,
                                uint32_t sector_offset,
                                std_error_t * const error);

int w25q32bv_flash_read_data_fast ( w25q32bv_flash_t const * const self,
                                    uint8_t *data,
                                    uint32_t size,
                                    uint32_t sector_number,
                                    uint32_t sector_offset,
                                    std_error_t * const error);

int w25q32bv_flash_enable_erasing_or_writing (  w25q32bv_flash_t const * const self,
                                                std_error_t * const error);

int w25q32bv_flash_write_page ( w25q32bv_flash_t const * const self,
                                uint8_t *data,
                                uint32_t size,
                                uint32_t page_number,
                                uint32_t page_offset,
                                std_error_t * const error);

int w25q32bv_flash_erase_sector (w25q32bv_flash_t const * const self, uint32_t sector_number, std_error_t * const error);
int w25q32bv_flash_erase_block (w25q32bv_flash_t const * const self, uint32_t block_number, std_error_t * const error);
int w25q32bv_flash_erase_chip (w25q32bv_flash_t const * const self, std_error_t * const error);

int w25q32bv_flash_wait_erasing_or_writing (w25q32bv_flash_t const * const self, std_error_t * const error);

int w25q32bv_flash_power_down (w25q32bv_flash_t const * const self, std_error_t * const error);
int w25q32bv_flash_release_power_down (w25q32bv_flash_t const * const self, std_error_t * const error);



// Private
typedef struct w25q32bv_flash
{
    w25q32bv_flash_config_t config;

    w25q32bv_flash_array_t array;

} w25q32bv_flash_t;

#endif // W25Q32BV_FLASH_H
