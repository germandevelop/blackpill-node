/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef NODE_TYPES_H
#define NODE_TYPES_H

#include <stddef.h>

#include "node/node.list.h"
#include "node/node.command.h"

typedef struct node_msg_header
{
    node_id_t source;
    node_id_t dest_array[NODE_LIST_SIZE];
    size_t dest_array_size;

} node_msg_header_t;

typedef struct node_msg
{
    node_msg_header_t header;

    // Payload - body
    node_command_id_t cmd_id;
    uint32_t value_0;
    int32_t value_1;
    float value_2;

} node_msg_t;

#endif // NODE_TYPES_H