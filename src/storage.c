/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#include "storage.h"

#include <assert.h>

#include "logger.h"
#include "std_error/std_error.h"


#define DEFAULT_ERROR_TEXT      "Storage error"
#define DEFAULT_LFS_ERROR_TEXT  "Storage lfs error"

#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


static int storage_lfs_block_device_read (const struct lfs_config *config, lfs_block_t sector_number, lfs_off_t sector_offset, void *raw_data, lfs_size_t size);
static int storage_lfs_block_device_prog (const struct lfs_config *config, lfs_block_t sector_number, lfs_off_t sector_offset, const void *raw_data, lfs_size_t size);
static int storage_lfs_block_device_erase (const struct lfs_config *config, lfs_block_t sector_number);
static int storage_lfs_block_device_sync (const struct lfs_config *config);

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

    LOG("Storage [w25q] : init (W25Q32BV)\r\n");

    w25q32bv_flash_config_t flash_config;
    flash_config.spi_lock_callback      = self->config.spi_lock_callback;
    flash_config.spi_unlock_callback    = self->config.spi_unlock_callback;
    flash_config.spi_select_callback    = self->config.spi_select_callback;
    flash_config.spi_unselect_callback  = self->config.spi_unselect_callback;
    flash_config.spi_tx_rx_callback     = self->config.spi_tx_rx_callback;
    flash_config.spi_timeout_ms         = self->config.spi_timeout_ms;
    flash_config.delay_callback         = self->config.delay_callback;

    w25q32bv_flash_init(&self->w25q32bv_flash, &flash_config);

    LOG("Storage [w25q] : release power down\r\n");

    int exit_code = w25q32bv_flash_release_power_down(&self->w25q32bv_flash, error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Storage [w25q] : %s\r\n", error->text);

        return exit_code;
    }

    LOG("Storage [w25q] : read info\r\n");

    w25q32bv_flash_array_t flash_array;
    w25q32bv_flash_get_array(&self->w25q32bv_flash, &flash_array);

    w25q32bv_flash_info_t flash_info;
    exit_code = w25q32bv_flash_read_info(&self->w25q32bv_flash, &flash_info, error);

    if (exit_code != STD_FAILURE)
    {
        LOG("Storage [w25q] : JEDEC ID = 0x%lX\r\n", flash_info.jedec_id);
        LOG("Storage [w25q] : capacity = %lu KBytes\r\n", flash_info.capacity_KByte);
    }
    else
    {
        LOG("Storage [w25q] : %s\r\n", error->text);

        return exit_code;
    }

    LOG("Storage [lfs] : init\r\n");

    const struct lfs_config cfg = { 0 };
    self->lfs_config = cfg;

    self->lfs_config.read       = storage_lfs_block_device_read;
    self->lfs_config.prog       = storage_lfs_block_device_prog;
    self->lfs_config.erase      = storage_lfs_block_device_erase;
    self->lfs_config.sync       = storage_lfs_block_device_sync;
    //self->lfs_config.lock       = storage_lfs_block_device_lock;
    //self->lfs_config.unlock     = storage_lfs_block_device_unlock;
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

    LOG("Storage [lfs] : mount\r\n");

    enum lfs_error lfs_error = (enum lfs_error)lfs_mount(&self->lfs, &self->lfs_config);

    if (lfs_error != LFS_ERR_OK)
    {
        LOG("Board [lfs] : format\r\n");

        lfs_format(&self->lfs, &self->lfs_config);
        lfs_error = (enum lfs_error)lfs_mount(&self->lfs, &self->lfs_config);
    }

    if (lfs_error == LFS_ERR_OK)
    {
        LOG("Storage [lfs] : mount success\r\n");
    }

    LOG("Storage [lfs] : unmount\r\n");

    lfs_unmount(&self->lfs);

    LOG("Storage [w25q] : power down\r\n");

    exit_code = w25q32bv_flash_power_down(&self->w25q32bv_flash, error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Storage [w25q] : %s\r\n", error->text);
    }

    return exit_code;
}


int storage_enable_power (storage_t * const self, std_error_t * const error)
{
    LOG("Storage [w25q] : release power down\r\n");

    const int exit_code = w25q32bv_flash_release_power_down(&self->w25q32bv_flash, error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Storage [w25q] : %s\r\n", error->text);
    }

    return exit_code;
}

int storage_disable_power (storage_t * const self, std_error_t * const error)
{
    LOG("Storage [w25q] : power down\r\n");

    const int exit_code = w25q32bv_flash_power_down(&self->w25q32bv_flash, error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Storage [w25q] : %s\r\n", error->text);
    }

    return exit_code;
}

int storage_mount_filesystem (storage_t * const self, std_error_t * const error)
{
    int exit_code = STD_SUCCESS;

    LOG("Storage [lfs] : mount\r\n");

    const enum lfs_error lfs_error = (enum lfs_error)lfs_mount(&self->lfs, &self->lfs_config);

    if (lfs_error != LFS_ERR_OK)
    {
        LOG("Storage [lfs] : mount failure = %d\r\n", lfs_error);

        exit_code = STD_FAILURE;
        std_error_catch_custom(error, (int)(lfs_error), DEFAULT_LFS_ERROR_TEXT, __FILE__, __LINE__);
    }

    return exit_code;
}

int storage_unmount_filesystem (storage_t * const self, std_error_t * const error)
{
    int exit_code = STD_SUCCESS;

    LOG("Storage [lfs] : unmount\r\n");

    const enum lfs_error lfs_error = (enum lfs_error)lfs_unmount(&self->lfs);

    if (lfs_error != LFS_ERR_OK)
    {
        LOG("Storage [lfs] : unmount failure = %d\r\n", lfs_error);

        exit_code = STD_FAILURE;
        std_error_catch_custom(error, (int)(lfs_error), DEFAULT_LFS_ERROR_TEXT, __FILE__, __LINE__);
    }

    return exit_code;
}


int storage_create_file (storage_t * const self, storage_file_t * const file, const char file_name[64], std_error_t * const error)
{
    int exit_code = STD_SUCCESS;

    LOG("Storage [lfs] : open file\r\n");

    file->config.buffer     = (void*)file->lfs_file_buffer;
    file->config.attr_count = 0U;

    enum lfs_error lfs_error = (enum lfs_error)lfs_file_opencfg(&self->lfs, &file->file, file_name, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC | LFS_O_APPEND, &file->config);

    if (lfs_error != LFS_ERR_OK)
    {
        LOG("Storage [lfs] : file error = %d\r\n", lfs_error);

        exit_code = STD_FAILURE;
        std_error_catch_custom(error, (int)(lfs_error), DEFAULT_LFS_ERROR_TEXT, __FILE__, __LINE__);
    }

    return exit_code;
}

int storage_open_file (storage_t * const self, storage_file_t * const file, const char file_name[64], std_error_t * const error)
{
    int exit_code = STD_SUCCESS;

    LOG("Storage [lfs] : open file\r\n");

    file->config.buffer     = (void*)file->lfs_file_buffer;
    file->config.attr_count = 0U;

    enum lfs_error lfs_error = (enum lfs_error)lfs_file_opencfg(&self->lfs, &file->file, file_name, LFS_O_RDONLY, &file->config);

    if (lfs_error != LFS_ERR_OK)
    {
        LOG("Storage [lfs] : file error = %d\r\n", lfs_error);

        exit_code = STD_FAILURE;
        std_error_catch_custom(error, (int)(lfs_error), DEFAULT_LFS_ERROR_TEXT, __FILE__, __LINE__);
    }

    return exit_code;
}

int storage_close_file (storage_t * const self, storage_file_t * const file, std_error_t * const error)
{
    int exit_code = STD_SUCCESS;

    LOG("Storage [lfs] : close file\r\n");

    enum lfs_error lfs_error = (enum lfs_error)lfs_file_close(&self->lfs, &file->file);

    if (lfs_error != LFS_ERR_OK)
    {
        LOG("Storage [lfs] : file error = %d\r\n", lfs_error);

        exit_code = STD_FAILURE;
        std_error_catch_custom(error, (int)(lfs_error), DEFAULT_LFS_ERROR_TEXT, __FILE__, __LINE__);
    }

    return exit_code;
}

int storage_remove_file (storage_t * const self, const char file_name[64], std_error_t * const error)
{
    int exit_code = STD_SUCCESS;

    LOG("Storage [lfs] : remove file\r\n");

    enum lfs_error lfs_error = (enum lfs_error)lfs_remove(&self->lfs, file_name);

    if (lfs_error != LFS_ERR_OK)
    {
        LOG("Storage [lfs] : file error = %d\r\n", lfs_error);

        exit_code = STD_FAILURE;
        std_error_catch_custom(error, (int)(lfs_error), DEFAULT_LFS_ERROR_TEXT, __FILE__, __LINE__);
    }

    return exit_code;
}


int storage_write_file (storage_t * const self, storage_file_t * const file, char const * const data, size_t size, std_error_t * const error)
{
    LOG("Storage [lfs] : write file\r\n");

    const lfs_soff_t bytes_written = lfs_file_write(&self->lfs, &file->file, (const void*)data, (lfs_size_t)size);

    if (bytes_written < 0)
    {
        LOG("Storage [lfs] : file error = %ld\r\n", bytes_written);

        std_error_catch_custom(error, (int)(bytes_written), DEFAULT_LFS_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    else
    {
        LOG("Board [lfs] : bytes written = %ld\r\n", bytes_written);
    }

    int exit_code = STD_SUCCESS;

    const enum lfs_error lfs_error = (enum lfs_error)lfs_file_sync(&self->lfs, &file->file);

    if (lfs_error != LFS_ERR_OK)
    {
        LOG("Storage [lfs] : file error = %d\r\n", lfs_error);

        exit_code = STD_FAILURE;
        std_error_catch_custom(error, (int)(lfs_error), DEFAULT_LFS_ERROR_TEXT, __FILE__, __LINE__);
    }

    return exit_code;
}

int storage_read_file (storage_t * const self, storage_file_t * const file, char *data, size_t * const size, size_t max_size, std_error_t * const error)
{
    int exit_code = STD_SUCCESS;

    LOG("Storage [lfs] : read file\r\n");

    *size = 0U;

    const lfs_ssize_t bytes_read = lfs_file_read(&self->lfs, &file->file, (void*)data, (lfs_size_t)max_size);

    if (bytes_read < 0)
    {
        LOG("Storage [lfs] : file error = %ld\r\n", bytes_read);

        exit_code = STD_FAILURE;
        std_error_catch_custom(error, (int)(bytes_read), DEFAULT_LFS_ERROR_TEXT, __FILE__, __LINE__);
    }
    else
    {
        LOG("Board [lfs] : bytes read = %ld\r\n", bytes_read);

        *size = (size_t)bytes_read;
    }

    return exit_code;
}

int storage_get_file_size (storage_t * const self, storage_file_t * const file, size_t * const size, std_error_t * const error)
{
    int exit_code = STD_SUCCESS;

    LOG("Storage [lfs] : get file size\r\n");

    *size = 0U;

    const lfs_soff_t file_size = lfs_file_size(&self->lfs, &file->file);

    if (file_size < 0)
    {
        LOG("Storage [lfs] : file error = %ld\r\n", file_size);

        exit_code = STD_FAILURE;
        std_error_catch_custom(error, (int)(file_size), DEFAULT_LFS_ERROR_TEXT, __FILE__, __LINE__);
    }
    else
    {
        LOG("Board [lfs] : bytes read = %ld\r\n", file_size);

        *size = (size_t)file_size;
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

    const int exit_code = w25q32bv_flash_read_data_fast(flash, (uint8_t*)raw_data, (uint32_t)size, (uint32_t)sector_number, (uint32_t)sector_offset, NULL);

    if (exit_code != STD_SUCCESS)
    {
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

    w25q32bv_flash_array_t flash_array;
    w25q32bv_flash_get_array(flash, &flash_array);

    uint32_t page_number = (((uint32_t)(sector_number) * flash_array.sector_size) / flash_array.page_size) + ((uint32_t)(sector_offset) / flash_array.page_size);
    uint32_t page_offset = (uint32_t)(sector_offset) % flash_array.page_size;

    uint8_t *data = (uint8_t*)raw_data;
    uint32_t data_size = (uint32_t)size;

    const uint32_t page_free_size = flash_array.page_size - page_offset;
    uint32_t size_to_write = data_size;

    if (data_size > page_free_size)
    {
        size_to_write = page_free_size;
    }

    while (true)
    {
        exit_code = w25q32bv_flash_enable_erasing_or_writing(flash, NULL);

        if (exit_code != STD_FAILURE)
        {
            exit_code = w25q32bv_flash_write_page(flash, data, size_to_write, page_number, page_offset, NULL);

            if (exit_code != STD_FAILURE)
            {
                exit_code = w25q32bv_flash_wait_erasing_or_writing(flash, NULL);

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
        return (int)(LFS_ERR_IO);
    }
    return (int)(LFS_ERR_OK);
}

int storage_lfs_block_device_erase (const struct lfs_config *config,
                                    lfs_block_t sector_number)
{
    const storage_t *storage = (const storage_t*)config->context;
    const w25q32bv_flash_t *flash = &storage->w25q32bv_flash;

    int exit_code = w25q32bv_flash_enable_erasing_or_writing(flash, NULL);

    if (exit_code != STD_FAILURE)
    {
        exit_code = w25q32bv_flash_erase_sector(flash, (uint32_t)sector_number, NULL);

        if (exit_code != STD_FAILURE)
        {
            exit_code = w25q32bv_flash_wait_erasing_or_writing(flash, NULL);
        }
    }

    if (exit_code != STD_SUCCESS)
    {
        return (int)(LFS_ERR_IO);
    }
    return (int)(LFS_ERR_OK);
}


int storage_lfs_block_device_sync (const struct lfs_config *config)
{
    UNUSED(config);

    return (int)(LFS_ERR_OK);
}

/*int storage_lfs_block_device_lock (const struct lfs_config *config)
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
}*/
