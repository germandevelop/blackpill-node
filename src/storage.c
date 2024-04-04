/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#include "storage.h"

#include <assert.h>

#include "lfs.h"

#include "devices/w25q32bv_flash.h"

#include "logger.h"
#include "std_error/std_error.h"


#define LFS_ERASE_CYCLES 500

#define DEFAULT_ERROR_TEXT "Storage error"


static int storage_lfs_block_device_read (const struct lfs_config *config, lfs_block_t sector_number, lfs_off_t sector_offset, void *raw_data, lfs_size_t size);
static int storage_lfs_block_device_prog (const struct lfs_config *config, lfs_block_t sector_number, lfs_off_t sector_offset, const void *raw_data, lfs_size_t size);
static int storage_lfs_block_device_erase (const struct lfs_config *config, lfs_block_t sector_number);
static int storage_lfs_block_device_sync (const struct lfs_config *config);
static int storage_lfs_block_device_lock (const struct lfs_config *config);
static int storage_lfs_block_device_unlock (const struct lfs_config *config);

int storage_init (storage_t * const self, storage_config_t const * const config, std_error_t * const error)
{
    assert(config                           != NULL);
    assert(config->spi_lock_callback        != NULL);
    assert(config->spi_unlock_callback      != NULL);
    assert(config->spi_select_callback      != NULL);
    assert(config->spi_unselect_callback    != NULL);
    assert(config->spi_tx_rx_callback       != NULL);
    assert(config->delay_callback           != NULL);

    self->config = *config;

    LOG("Storage [flash] : init (W25Q32BV)\r\n");

    w25q32bv_flash_config_t flash_config;
    flash_config.spi_select_callback    = self->config.spi_select_callback;
    flash_config.spi_unselect_callback  = self->config.spi_unselect_callback;
    flash_config.spi_tx_rx_callback     = self->config.spi_tx_rx_callback;
    flash_config.spi_timeout_ms         = self->config.spi_timeout_ms;
    flash_config.delay_callback         = self->config.delay_callback;

    w25q32bv_flash_init(&self->w25q32bv_flash, &flash_config);

    LOG("Storage [flash] : release power down\r\n");

    int exit_code = w25q32bv_flash_release_power_down(&self->w25q32bv_flash, error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Storage [flash] : %s\r\n", error->text);

        return exit_code;
    }

    LOG("Storage [flash] : read info\r\n");

    w25q32bv_flash_array_t flash_array;
    w25q32bv_flash_get_array(&self->w25q32bv_flash, &flash_array);

    w25q32bv_flash_info_t flash_info;
    exit_code = w25q32bv_flash_read_info(&self->w25q32bv_flash, &flash_info, error);

    if (exit_code != STD_FAILURE)
    {
        LOG("Storage [flash] : JEDEC ID = 0x%lX\r\n", flash_info.jedec_id);
        LOG("Storage [flash] : capacity = %lu KBytes\r\n", flash_info.capacity_KByte);
    }
    else
    {
        LOG("Storage [flash] : %s\r\n", error->text);

        return exit_code;
    }

    LOG("Storage [lfs] : init\r\n");

    self->lfs_config.read       = storage_lfs_block_device_read;
    self->lfs_config.prog       = storage_lfs_block_device_prog;
    self->lfs_config.erase      = storage_lfs_block_device_erase;
    self->lfs_config.sync       = storage_lfs_block_device_sync;
    self->lfs_config.lock       = storage_lfs_block_device_lock;
    self->lfs_config.unlock     = storage_lfs_block_device_unlock;
    self->lfs_config.context    = (void*)self;
    
    self->lfs_config.read_size      = LFS_MIN_READ_BLOCK_SIZE;
    self->lfs_config.prog_size      = LFS_MIN_PROG_BLOCK_SIZE;
    self->lfs_config.block_size     = flash_array.sector_size;
    self->lfs_config.block_count    = flash_array.sector_count;
    self->lfs_config.cache_size     = LFS_CACHE_SIZE;
    self->lfs_config.lookahead_size = LFS_LOOKAHEAD_SIZE;
    self->lfs_config.block_cycles   = LFS_ERASE_CYCLES;

    self->lfs_config.read_buffer        = self->lfs_read_buffer;
    self->lfs_config.prog_buffer        = self->lfs_prog_buffer;
    self->lfs_config.lookahead_buffer   = self->lfs_lookahead_buffer;

    lfs_t lfs;

    enum lfs_error lfs_error = (enum lfs_error)lfs_mount(&lfs, &self->lfs_config);

    if (lfs_error != LFS_ERR_OK)
    {
        LOG("Board [lfs] : format\r\n");

        lfs_format(&lfs, &self->lfs_config);
        lfs_error = (enum lfs_error)lfs_mount(&lfs, &self->lfs_config);
    }

    if (lfs_error == LFS_ERR_OK)
    {
        LOG("Storage [lfs] : mount success\r\n");
    }

    lfs_unmount(&lfs);

    LOG("Storage [flash] : power down\r\n");

    exit_code = w25q32bv_flash_power_down(&self->w25q32bv_flash, error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Storage [flash] : %s\r\n", error->text);
    }

    return exit_code;
}


int storage_lfs_block_device_read ( const struct lfs_config *config,
                                    lfs_block_t sector_number,
                                    lfs_off_t sector_offset,
                                    void *raw_data,
                                    lfs_size_t size)
{
    const storage_t *storage = (const storage_t*)config->context;
    const w25q32bv_flash_t *flash = &storage->w25q32bv_flash;

    std_error_t error;
    std_error_init(&error);

    const int exit_code = w25q32bv_flash_read_data_fast(flash, (uint8_t*)raw_data, (uint32_t)size, (uint32_t)sector_number, (uint32_t)sector_offset, NULL);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Storage [flash] : %s\r\n", error.text);

        return (int)(LFS_ERR_IO);
    }
    return (int)(LFS_ERR_OK);
}

int storage_lfs_block_device_prog ( const struct lfs_config *config,
                                    lfs_block_t sector_number,
                                    lfs_off_t sector_offset,
                                    const void *raw_data,
                                    lfs_size_t size)
{
    const storage_t *storage = (const storage_t*)config->context;
    const w25q32bv_flash_t *flash = &storage->w25q32bv_flash;

    int exit_code = STD_FAILURE;

    std_error_t error;
    std_error_init(&error);


    w25q32bv_flash_array_t flash_array;
    w25q32bv_flash_get_array(flash, &flash_array);

    uint32_t page_number = (((uint32_t)(sector_number) * flash_array.sector_size) / flash_array.page_size) + ((uint32_t)(sector_offset) / flash_array.page_size);
    uint32_t page_offset = (uint32_t)(sector_offset) % flash_array.page_size;

    uint8_t *data = (uint8_t *)raw_data;
    uint32_t data_size = (uint32_t)size;

    const uint32_t page_free_size = flash_array.page_size - page_offset;
    uint32_t size_to_write = data_size;

    if (data_size > page_free_size)
    {
        size_to_write = page_free_size;
    }

    while (true)
    {
        exit_code = w25q32bv_flash_enable_erasing_or_writing(flash, &error);

        if (exit_code != STD_FAILURE)
        {
            exit_code = w25q32bv_flash_write_page(flash, data, size_to_write, page_number, page_offset, &error);

            if (exit_code != STD_FAILURE)
            {
                exit_code = w25q32bv_flash_wait_erasing_or_writing(flash, &error);

                if (exit_code != STD_FAILURE)
                {
                    if (data_size > size_to_write)
                    {
                        ++page_number;
                        page_offset = 0U;
                        data += (uint8_t)(size_to_write);
                        data_size -= size_to_write;

                        if (data_size > flash_array.page_size)
                        {
                            size_to_write = flash_array.page_size;
                        }
                        else
                        {
                            size_to_write = data_size;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        if (exit_code != STD_SUCCESS)
        {
            break;
        }
    }

    if (exit_code != STD_SUCCESS)
    {
        LOG("Storage [flash] : %s\r\n", error.text);

        return (int)(LFS_ERR_IO);
    }
    return (int)(LFS_ERR_OK);
}

int storage_lfs_block_device_erase (const struct lfs_config *config,
                                    lfs_block_t sector_number)
{
    const storage_t *storage = (const storage_t*)config->context;
    const w25q32bv_flash_t *flash = &storage->w25q32bv_flash;

    std_error_t error;
    std_error_init(&error);

    int exit_code = w25q32bv_flash_enable_erasing_or_writing(flash, &error);

    if (exit_code != STD_FAILURE)
    {
        exit_code = w25q32bv_flash_erase_sector(flash, (uint32_t)sector_number, &error);

        if (exit_code != STD_FAILURE)
        {
            exit_code = w25q32bv_flash_wait_erasing_or_writing(flash, &error);
        }
    }

    if (exit_code != STD_SUCCESS)
    {
        LOG("Storage [flash] : %s\r\n", error.text);

        return (int)(LFS_ERR_IO);
    }
    return (int)(LFS_ERR_OK);
}


int storage_lfs_block_device_sync (const struct lfs_config *config)
{
    return (int)(LFS_ERR_OK);
}

int storage_lfs_block_device_lock (const struct lfs_config *config)
{
    const storage_t *storage = (const storage_t*)config->context;

    storage->config.spi_lock_callback();

    return (int)(LFS_ERR_OK);
}

int storage_lfs_block_device_unlock (const struct lfs_config *config)
{
    const storage_t *storage = (const storage_t*)config->context;

    storage->config.spi_unlock_callback();

    return (int)(LFS_ERR_OK);
}
