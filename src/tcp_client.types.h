/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef TCP_CLIENT_TYPES_H
#define TCP_CLIENT_TYPES_H

#include <stddef.h>

typedef struct tcp_msg
{
    char data[128];
    size_t size;

} tcp_msg_t;

#endif // TCP_CLIENT_TYPES_H
