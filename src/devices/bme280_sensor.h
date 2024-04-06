/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef BME280_SENSOR_H
#define BME280_SENSOR_H

#include <stdint.h>

typedef struct std_error std_error_t;

typedef int (*bme280_sensor_i2c_callback_t) (uint16_t device_address, uint16_t register_address, uint16_t register_size, uint8_t *array, uint16_t array_size, uint32_t timeout_ms, std_error_t * const error);
typedef void (*bme280_sensor_delay_callback_t) (uint32_t delay_ms);

typedef struct bme280_sensor_config
{
    bme280_sensor_i2c_callback_t read_i2c_callback;
    bme280_sensor_i2c_callback_t write_i2c_callback;
    uint32_t i2c_timeout_ms;
    
    bme280_sensor_delay_callback_t delay_callback;

} bme280_sensor_config_t;

typedef struct bme280_sensor_data
{
    float pressure_hPa;
    float temperature_C;
    float humidity_pct;

} bme280_sensor_data_t;

int bme280_sensor_init (bme280_sensor_config_t const * const init_config, std_error_t * const error);

int bme280_sensor_read_data (bme280_sensor_data_t * const data, std_error_t * const error);

#endif // BME280_SENSOR_H
