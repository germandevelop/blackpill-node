/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef NODE_MAPPER_H
#define NODE_MAPPER_H

#include <stddef.h>

typedef struct node_msg node_msg_t;
typedef struct std_error std_error_t;

void node_mapper_serialize_message (node_msg_t const * const msg, char *raw_data, size_t * const raw_data_size);
int node_mapper_deserialize_message (const char *raw_data, node_msg_t * const msg, std_error_t * const error);

#endif // NODE_MAPPER_H
