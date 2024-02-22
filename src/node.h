/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef NODE_H
#define NODE_H

#include "node/node.list.h"

typedef struct node_msg node_msg_t;
typedef struct tcp_msg tcp_msg_t;
typedef struct std_error std_error_t;

typedef void (*node_send_tcp_msg_callback_t) (tcp_msg_t const * const send_msg);
typedef void (*node_receive_msg_callback_t) (node_msg_t const * const msg);

typedef struct node_config
{
    node_id_t id;

    node_send_tcp_msg_callback_t send_tcp_msg_callback;
    node_receive_msg_callback_t receive_msg_callback;

} node_config_t;

int node_init (node_config_t const * const init_config, std_error_t * const error);

int node_send_msg (node_msg_t const * const send_msg, std_error_t * const error);
int node_receive_tcp_msg (tcp_msg_t const * const recv_msg, std_error_t * const error);

#endif // NODE_H
