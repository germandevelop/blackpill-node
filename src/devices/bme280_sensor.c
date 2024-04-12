/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "bme280_sensor.h"

#include <assert.h>

#include "bme280.h"

#include "std_error/std_error.h"


#define UNUSED(x) (void)(x)


static bme280_sensor_config_t config;

static struct bme280_dev device;
static uint32_t delay_ms;


static BME280_INTF_RET_TYPE bme280_sensor_read_i2c (uint8_t register_address, uint8_t *array, uint32_t array_size, void *user_data);
static BME280_INTF_RET_TYPE bme280_sensor_write_i2c (uint8_t register_address, const uint8_t *array, uint32_t array_size, void *user_data);
static void bme280_sensor_delay_us (uint32_t period_us, void *user_data);

int bme280_sensor_init (bme280_sensor_config_t const * const init_config, std_error_t * const error)
{
    assert(init_config != NULL);
    assert(init_config->lock_i2c_callback   != NULL);
    assert(init_config->unlock_i2c_callback != NULL);
    assert(init_config->read_i2c_callback   != NULL);
    assert(init_config->write_i2c_callback  != NULL);
    assert(init_config->delay_callback      != NULL);

    config = *init_config;

    delay_ms = 4U * 1000U;

    device.intf     = BME280_I2C_INTF;
    device.read     = bme280_sensor_read_i2c;
    device.write    = bme280_sensor_write_i2c;
    device.delay_us = bme280_sensor_delay_us;
    device.intf_ptr = (void*)error;

    int8_t exit_code = bme280_init(&device);

    if (exit_code != BME280_OK)
    {
        return STD_FAILURE;
    }

    device.settings.osr_h   = BME280_OVERSAMPLING_1X;
    device.settings.osr_p   = BME280_OVERSAMPLING_16X;
    device.settings.osr_t   = BME280_OVERSAMPLING_2X;
    device.settings.filter  = BME280_FILTER_COEFF_16;

    const uint8_t settings = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

    exit_code = bme280_set_sensor_settings(settings, &device);

    if (exit_code != BME280_OK)
    {
        return STD_FAILURE;
    }

    delay_ms = bme280_cal_meas_delay(&device.settings) + 100U;

    exit_code = bme280_set_sensor_mode(BME280_SLEEP_MODE, &device);

    if (exit_code != BME280_OK)
    {
        return STD_FAILURE;
    }

    return STD_SUCCESS;
}

int bme280_sensor_read_data (bme280_sensor_data_t * const data, std_error_t * const error)
{
    device.intf_ptr = (void*)error;

    int8_t exit_code = bme280_set_sensor_mode(BME280_FORCED_MODE, &device);

    if (exit_code != BME280_OK)
    {
        return STD_FAILURE;
    }

    config.delay_callback(delay_ms);

    struct bme280_data sensor_data;
    exit_code = bme280_get_sensor_data(BME280_ALL, &sensor_data, &device);

    if (exit_code != BME280_OK)
    {
        return STD_FAILURE;
    }

    data->temperature_C = (float)sensor_data.temperature;
    data->pressure_hPa  = (float)(0.01 * sensor_data.pressure);
    data->humidity_pct  = (float)sensor_data.humidity;

    return STD_SUCCESS;
}


BME280_INTF_RET_TYPE bme280_sensor_read_i2c (uint8_t register_address, uint8_t *array, uint32_t array_size, void *user_data)
{
    std_error_t *error = (std_error_t*)user_data;

    config.lock_i2c_callback();
    int exit_code = config.read_i2c_callback(BME280_I2C_ADDR_PRIM, (uint16_t)register_address, sizeof(register_address), array, (uint16_t)array_size, config.i2c_timeout_ms, error);
    config.unlock_i2c_callback();

    return exit_code;
}

BME280_INTF_RET_TYPE bme280_sensor_write_i2c (uint8_t register_address, const uint8_t *array, uint32_t array_size, void *user_data)
{
    std_error_t *error = (std_error_t*)user_data;

    config.lock_i2c_callback();
    int exit_code = config.write_i2c_callback(BME280_I2C_ADDR_PRIM, (uint16_t)register_address, sizeof(register_address), (uint8_t*)array, (uint16_t)array_size, config.i2c_timeout_ms, error);
    config.unlock_i2c_callback();

    return exit_code;
}

void bme280_sensor_delay_us (uint32_t period_us, void *user_data)
{
    UNUSED(user_data);

    config.delay_callback((period_us / 1000U) + 1U);

    return;
}
