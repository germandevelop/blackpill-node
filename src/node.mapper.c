/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "node.mapper.h"
#include "node.type.h"

#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

#include "lwjson/lwjson.h"

#include "std_error/std_error.h"


#define DEFAULT_ERROR_TEXT  "Mapper error"


void node_mapper_serialize_message (node_msg_t const * const msg, char *raw_data, size_t * const raw_data_size)
{
    assert(raw_data         != NULL);
    assert(msg              != NULL);
    assert(raw_data_size    != NULL);
    assert(msg->header.dest_array_size != 0U);

    char dest_array[16] = { '\0' };

    sprintf(dest_array, "%d", msg->header.dest_array[0]);

    for (size_t i = 1U; i < msg->header.dest_array_size; ++i)
    {
        sprintf(dest_array + strlen(dest_array), ",%d", msg->header.dest_array[i]);
    }

    int data_size = (-1);

    if ((msg->cmd_id == SET_MODE) || (msg->cmd_id == SET_LIGHT) || (msg->cmd_id == SET_INTRUSION))
    {
        data_size = sprintf(raw_data, "{\"src_id\":%d,\"dst_id\":[%s],\"cmd_id\":%d,\"data\":{\"value_id\":%ld}}", msg->header.source, dest_array, msg->cmd_id, msg->value_0);
    }
    else if (msg->cmd_id == UPDATE_TEMPERATURE)
    {
        data_size = sprintf(raw_data, "{\"src_id\":%d,\"dst_id\":[%s],\"cmd_id\":%d,\"data\":{\"pres_hpa\":%lu,\"hum_pct\":%ld,\"temp_c\":%.1f}}", msg->header.source, dest_array, msg->cmd_id, msg->value_0, msg->value_1, msg->value_2);
    }
    else if (msg->cmd_id == UPDATE_DOOR_STATE)
    {
        data_size = sprintf(raw_data, "{\"src_id\":%d,\"dst_id\":[%s],\"cmd_id\":%d,\"data\":{\"door_state\":%lu}}", msg->header.source, dest_array, msg->cmd_id, msg->value_0);
    }
    else
    {
        data_size = sprintf(raw_data, "{\"src_id\":%d,\"dst_id\":[%s],\"cmd_id\":%d}", msg->header.source, dest_array, DO_NOTHING);
    }

    *raw_data_size = 0U;

    if (data_size > 0)
    {
        *raw_data_size = (size_t)data_size;
    }

    return;
}

int node_mapper_deserialize_message (const char *raw_data, node_msg_t * const msg, std_error_t * const error)
{
    assert(raw_data != NULL);
    assert(msg      != NULL);

    msg->header.dest_array_size = 0U;

    int exit_code = STD_SUCCESS;

    static lwjson_t lwjson;
    static lwjson_token_t tokens[8];

    lwjson_init(&lwjson, tokens, LWJSON_ARRAYSIZE(tokens));

    lwjsonr_t parser_code = lwjson_parse(&lwjson, raw_data);

    if (parser_code == lwjsonOK)
    {
        const lwjson_token_t *token;

        bool is_token_parsed = ((token = lwjson_find(&lwjson, "src_id")) != NULL) && (token->type == LWJSON_TYPE_NUM_INT);

        if (is_token_parsed == true)
        {
            msg->header.source = (node_id_t)token->u.num_int;
        }

        is_token_parsed = ((token = lwjson_find(&lwjson, "dst_id")) != NULL) && (token->type == LWJSON_TYPE_ARRAY);

        if (is_token_parsed == true)
        {
            for (const lwjson_token_t *tkn = lwjson_get_first_child(token); tkn != NULL; tkn = tkn->next)
            {
                msg->header.dest_array[msg->header.dest_array_size] = (node_id_t)tkn->u.num_int;
                ++msg->header.dest_array_size;
            }
        }

        is_token_parsed = ((token = lwjson_find(&lwjson, "cmd_id")) != NULL) && (token->type == LWJSON_TYPE_NUM_INT);

        if (is_token_parsed == true)
        {
            msg->cmd_id = (node_command_id_t)token->u.num_int;
        }
        else
        {
            msg->cmd_id = DO_NOTHING;
        }

        if ((msg->cmd_id == SET_MODE) || (msg->cmd_id == SET_LIGHT) || (msg->cmd_id == SET_INTRUSION))
        {
            is_token_parsed = ((token = lwjson_find(&lwjson, "data")) != NULL) && (token->type == LWJSON_TYPE_OBJECT);

            if (is_token_parsed == true)
            {
                const lwjson_token_t *tkn = lwjson_find_ex(&lwjson, token, "value_id");

                is_token_parsed = (tkn!= NULL) && (tkn->type == LWJSON_TYPE_NUM_INT);

                if (is_token_parsed == true)
                {
                    msg->value_0 = tkn->u.num_int;
                }
            }
        }
    }
    else
    {
        std_error_catch_custom(error, (int)parser_code, DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        exit_code = STD_FAILURE;
    }
    lwjson_free(&lwjson);

    return exit_code;
}
