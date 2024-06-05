/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "logger.h"


#define UNUSED(x) (void)(x)


static logger_config_t config;


void logger_init (logger_config_t const * const init_config)
{
    config = *init_config;

    return;
}

// Redefine
//#ifndef NDEBUG

int _write(int fd, char *ptr, int len)
{
    UNUSED(fd);
    
    if (config.write_array_callback != NULL)
    {
        config.write_array_callback((const uint8_t*)ptr, (uint16_t)len);
    }
    return len;
}

//#endif // NDEBUG