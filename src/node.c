/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "node.h"
#include "node.type.h"
#include "node.mapper.h"

#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "tcp_client.type.h"

#include "logger.h"
#include "std_error/std_error.h"


#define RTOS_TASK_STACK_SIZE    512U    // 512*4=2048 bytes
#define RTOS_TASK_PRIORITY      2U      // 0 - lowest, 4 - highest
#define RTOS_TASK_NAME          "node"  // 16 - max length

#define RTOS_QUEUE_TICKS_TO_WAIT    (1U * 100U)

#define MSG_BUFFER_SIZE     8U
#define TCP_MSG_BUFFER_SIZE 4U

#define DEFAULT_ERROR_TEXT  "Node error"
#define MAPPER_ERROR_TEXT   "Node mapper error"
#define QUEUE_ERROR_TEXT    "Node queue error"
#define MALLOC_ERROR_TEXT   "Node memory allocation error"

#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


static TaskHandle_t task;
static QueueHandle_t work_msg_queue;
static QueueHandle_t free_msg_queue;
static QueueHandle_t work_tcp_msg_queue;
static QueueHandle_t free_tcp_msg_queue;
static QueueSetHandle_t msg_queue_set;

static node_config_t config;

static node_msg_t *msg_buffer;
static tcp_msg_t *tcp_msg_buffer;


static int node_malloc (std_error_t * const error);
static void node_task (void *parameters);

int node_init (node_config_t const * const init_config, std_error_t * const error)
{
    assert(init_config                          != NULL);
    assert(init_config->receive_msg_callback    != NULL);
    assert(init_config->send_tcp_msg_callback   != NULL);

    config = *init_config;

    return node_malloc(error);
}

int node_send_msg (node_msg_t const * const send_msg, std_error_t * const error)
{
    node_msg_t *free_msg;

    if (xQueueReceive(free_msg_queue, (void*)&free_msg, RTOS_QUEUE_TICKS_TO_WAIT) != pdPASS)
    {
        std_error_catch_custom(error, STD_FAILURE, QUEUE_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    memcpy((void*)(free_msg), (const void*)(send_msg), sizeof(node_msg_t));

    xQueueSend(work_msg_queue, (const void*)&free_msg, RTOS_QUEUE_TICKS_TO_WAIT);

    return STD_SUCCESS;
}

int node_receive_tcp_msg (tcp_msg_t const * const recv_msg, std_error_t * const error)
{
    tcp_msg_t *free_tcp_msg;

    if (xQueueReceive(free_tcp_msg_queue, (void*)&free_tcp_msg, RTOS_QUEUE_TICKS_TO_WAIT) != pdPASS)
    {
        std_error_catch_custom(error, STD_FAILURE, QUEUE_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    memcpy((void*)(free_tcp_msg), (const void*)(recv_msg), sizeof(tcp_msg_t));

    xQueueSend(work_tcp_msg_queue, (const void*)&free_tcp_msg, RTOS_QUEUE_TICKS_TO_WAIT);

    return STD_SUCCESS;
}


void node_task (void *parameters)
{
    UNUSED(parameters);
    
    std_error_t error;
    std_error_init(&error);

    xQueueAddToSet(work_msg_queue, msg_queue_set);
    xQueueAddToSet(work_tcp_msg_queue, msg_queue_set);

    for (size_t i = 0U; i < MSG_BUFFER_SIZE; ++i)
    {
        node_msg_t *free_msg = &msg_buffer[i];

        xQueueSend(free_msg_queue, (const void*)&free_msg, RTOS_QUEUE_TICKS_TO_WAIT);
    }

    for (size_t i = 0U; i < TCP_MSG_BUFFER_SIZE; ++i)
    {
        tcp_msg_t *free_tcp_msg = &tcp_msg_buffer[i];

        xQueueSend(free_tcp_msg_queue, (const void*)&free_tcp_msg, RTOS_QUEUE_TICKS_TO_WAIT);
    }

    while (true)
    {
        QueueSetMemberHandle_t activated_queue = xQueueSelectFromSet(msg_queue_set, portMAX_DELAY);

        // Process TCP message
        if (activated_queue == work_tcp_msg_queue)
        {
            tcp_msg_t *work_tcp_msg;

            if(xQueueReceive(work_tcp_msg_queue, (void*)&work_tcp_msg, 0U) == pdPASS)
            {
                LOG("Node : input tcp message - %s\r\n", work_tcp_msg->data);

                node_msg_t node_msg;

                if (node_mapper_deserialize_message(work_tcp_msg->data, &node_msg, &error) == STD_SUCCESS)
                {
                    bool is_dest_node = false;

                    for (size_t i = 0U;i < node_msg.header.dest_array_size; ++i)
                    {
                        if (node_msg.header.dest_array[i] == config.id)
                        {
                            is_dest_node = true;

                            break;
                        }
                    }

                    if (is_dest_node == true)
                    {
                        node_msg_t *free_msg;

                        if (xQueueReceive(free_msg_queue, (void*)&free_msg, RTOS_QUEUE_TICKS_TO_WAIT) == pdPASS)
                        {
                            memcpy((void*)(free_msg), (const void*)(&node_msg), sizeof(node_msg_t));

                            xQueueSend(work_msg_queue, (const void*)&free_msg, RTOS_QUEUE_TICKS_TO_WAIT);
                        }
                    }
                }
                else
                {
                    LOG("Node : %s\r\n", error.text);
                }
            }

            xQueueSend(free_tcp_msg_queue, (const void*)&work_tcp_msg, RTOS_QUEUE_TICKS_TO_WAIT);
        }

        // Process node message
        {
            node_msg_t *work_msg;

            if(xQueueReceive(work_msg_queue, (void*)&work_msg, 0U) == pdPASS)
            {
                if (work_msg->header.source != config.id)
                {
                    tcp_msg_t send_tcp_msg;

                    node_mapper_serialize_message(work_msg, send_tcp_msg.data, &send_tcp_msg.size);

                    LOG("Node : output tcp message - %s\r\n", send_tcp_msg.data);

                    config.send_tcp_msg_callback(&send_tcp_msg);
                }
                else
                {
                    config.receive_msg_callback(work_msg);
                }

                xQueueSend(free_msg_queue, (const void*)&work_msg, RTOS_QUEUE_TICKS_TO_WAIT);
            }
        }
    }

    return;
}


int node_malloc (std_error_t * const error)
{
    msg_buffer      = (node_msg_t*)pvPortMalloc(MSG_BUFFER_SIZE * sizeof(node_msg_t));
    tcp_msg_buffer  = (tcp_msg_t*)pvPortMalloc(TCP_MSG_BUFFER_SIZE * sizeof(tcp_msg_t));

    const bool are_buffers_allocated = (msg_buffer != NULL) && (tcp_msg_buffer != NULL);

    work_msg_queue      = xQueueCreate(MSG_BUFFER_SIZE, sizeof(node_msg_t*));
    free_msg_queue      = xQueueCreate(MSG_BUFFER_SIZE, sizeof(node_msg_t*));
    work_tcp_msg_queue  = xQueueCreate(TCP_MSG_BUFFER_SIZE, sizeof(tcp_msg_t*));
    free_tcp_msg_queue  = xQueueCreate(TCP_MSG_BUFFER_SIZE, sizeof(tcp_msg_t*));

    const bool are_queues_allocated = (work_msg_queue != NULL) && (free_msg_queue != NULL) &&
                                        (work_tcp_msg_queue != NULL) && (free_tcp_msg_queue != NULL);

    msg_queue_set = xQueueCreateSet(MSG_BUFFER_SIZE + TCP_MSG_BUFFER_SIZE);

    const bool are_queue_sets_allocated = (msg_queue_set != NULL);

    if ((are_buffers_allocated != true) || (are_queues_allocated != true) || (are_queue_sets_allocated != true))
    {
        vPortFree((void*)msg_buffer);
        vPortFree((void*)tcp_msg_buffer);
        vQueueDelete(work_msg_queue);
        vQueueDelete(free_msg_queue);
        vQueueDelete(work_tcp_msg_queue);
        vQueueDelete(free_tcp_msg_queue);

        std_error_catch_custom(error, STD_FAILURE, MALLOC_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    BaseType_t exit_code = xTaskCreate(node_task, RTOS_TASK_NAME, RTOS_TASK_STACK_SIZE, NULL, RTOS_TASK_PRIORITY, &task);

    if (exit_code != pdPASS)
    {
        std_error_catch_custom(error, (int)exit_code, MALLOC_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}
