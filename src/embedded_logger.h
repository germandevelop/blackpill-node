/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef EMBEDDED_LOGGER_H
#define EMBEDDED_LOGGER_H

#include <stdint.h>

typedef void (*write_array_callback_t) (const uint8_t *data, uint16_t data_size);

typedef struct embedded_logger_config
{
    write_array_callback_t write_array_callback;

} embedded_logger_config_t;

void embedded_logger_init (embedded_logger_config_t const * const init_config);

#ifdef NDEBUG
#define LOG(...) ((void)0U)
#else
#include <stdio.h>
#define LOG(...) printf(__VA_ARGS__)
#endif // NDEBUG

#endif // EMBEDDED_LOGGER_H
