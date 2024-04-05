/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#ifndef STOARGE_H
#define STOARGE_H

#include <stdint.h>

typedef struct std_error std_error_t;

typedef void (*storage_spi_lock_callback_t) ();
typedef void (*storage_spi_select_callback_t) ();
typedef int (*storage_spi_tx_rx_callback_t) (uint8_t *tx_data, uint8_t *rx_data, uint16_t size,
                                                uint32_t timeout_ms, std_error_t * const error);
typedef void (*storage_delay_callback_t) (uint32_t delay_ms);

typedef struct storage_config
{
    storage_spi_lock_callback_t spi_lock_callback;
    storage_spi_lock_callback_t spi_unlock_callback;
    storage_spi_select_callback_t spi_select_callback;
    storage_spi_select_callback_t spi_unselect_callback;
    storage_spi_tx_rx_callback_t spi_tx_rx_callback;
    uint32_t spi_timeout_ms;

    storage_delay_callback_t delay_callback;

} storage_config_t;

typedef struct storage storage_t;

int storage_init (storage_t * const self, storage_config_t const * const config, std_error_t * const error);



// Private
#include "lfs.h"
#include "devices/w25q32bv_flash.h"

#define LFS_MIN_READ_BLOCK_SIZE 16U
#define LFS_MIN_PROG_BLOCK_SIZE 16U
#define LFS_CACHE_SIZE          512U
#define LFS_LOOKAHEAD_SIZE      128U
#define LFS_ERASE_CYCLES        500

typedef struct storage
{
    storage_config_t config;

    w25q32bv_flash_t w25q32bv_flash;

    struct lfs_config lfs_config;
    uint8_t lfs_read_buffer[LFS_CACHE_SIZE];
    uint8_t lfs_prog_buffer[LFS_CACHE_SIZE];
    uint8_t lfs_lookahead_buffer[LFS_LOOKAHEAD_SIZE];
    uint8_t lfs_file_buffer[LFS_CACHE_SIZE];

} storage_t;

#endif // STOARGE_H
