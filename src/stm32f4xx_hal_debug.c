/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifdef USE_FULL_ASSERT

#include "logger.h"

void assert_failed (uint8_t *file, uint32_t line)
{
    LOG("Wrong parameters value: file %s on line %lu\r\n", file, line);

    return;
}

#endif // USE_FULL_ASSERT