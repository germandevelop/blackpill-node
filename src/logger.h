/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdint.h>

typedef void (*write_array_callback_t) (const uint8_t *data, uint16_t data_size);

typedef struct logger_config
{
    write_array_callback_t write_array_callback;

} logger_config_t;

void logger_init (logger_config_t const * const init_config);

//#ifdef NDEBUG
//#define LOG(...) ((void)0U)
//#else
#define LOG(...) printf(__VA_ARGS__)
//#endif // NDEBUG

#endif // LOGGER_H
