/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "node.h"

#include <stdbool.h>
#include <limits.h>
#include <assert.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"

#include "lwjson/lwjson.h"

#include "embedded_logger.h"
#include "std_error/std_error.h"


#define RTOS_TASK_STACK_SIZE    512U    // 512*4=2048 bytes
#define RTOS_TASK_PRIORITY      3U      // 0 - lowest, 4 - highest
#define RTOS_TASK_NAME          "node"  // 16 - max length

//#define LIGHT_LEVEL_PERIOD_MIN      (5U * 60U * 1000U) // 5 min
#define LIGHT_LEVEL_PERIOD_MIN      (30U * 1000U) // 5 min
#define REMOTE_BUTTON_QUEUE_SIZE    4U

#define DEFAULT_ERROR_TEXT  "Node error"
#define MALLOC_ERROR_TEXT   "Node memory allocation error"

#define REMOTE_BUTTON_NOTIFICATION   (1 << 0)

#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


static TaskHandle_t task;
static QueueHandle_t remote_button_queue;
static TimerHandle_t light_level_timer;

static node_config_t config;


static int node_malloc (std_error_t * const error);
static void node_task (void *parameters);
static void node_light_level_timer (TimerHandle_t timer);

int node_init (node_config_t const * const init_config, std_error_t * const error)
{
    assert(init_config != NULL);
    assert(init_config->get_light_level_callback != NULL);
    assert(init_config->set_led_color_callback != NULL);

    config = *init_config;

    return node_malloc(error);
}

void node_remote_control_ISR (node_remote_button_t remote_button)
{
    BaseType_t is_higher_priority_task_woken;

    xQueueSendFromISR(remote_button_queue, (const void*)&remote_button, &is_higher_priority_task_woken);

    xTaskNotifyFromISR(task, REMOTE_BUTTON_NOTIFICATION, eSetBits, &is_higher_priority_task_woken);

    portYIELD_FROM_ISR(is_higher_priority_task_woken);

    return;
}

void node_task (void *parameters)
{
    UNUSED(parameters);
    
    std_error_t error;
    std_error_init(&error);

    BaseType_t exit_code = xTimerStart(light_level_timer, 5U * 1000U);

    if (exit_code != pdPASS)
    {
        std_error_catch_custom(&error, (int)exit_code, MALLOC_ERROR_TEXT, __FILE__, __LINE__);

        LOG("%s\r\n", error.text);
    }

    while (true)
    {
        uint32_t notification;
        xTaskNotifyWait(0U, ULONG_MAX, &notification, 30U * 1000U);

        if ((notification & REMOTE_BUTTON_NOTIFICATION) != 0U)
        {
            node_remote_button_t remote_button;

            while (xQueueReceive(remote_button_queue, (void*)&remote_button, (TickType_t)0U) == pdPASS)
            {
                LOG("Node: remote button - %i\r\n", remote_button);
            }
        }
    }

    return;
}

void node_light_level_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    //config.set_led_color_callback(NO_COLOR, NULL);

    //vTaskDelay(1U * 1000U);

    uint32_t light_level = 0U;
    config.get_light_level_callback(&light_level, NULL);

    //config.set_led_color_callback(GREEN_COLOR, NULL);

    LOG("Node: light level - %lu\r\n", light_level);

    return;
}

int node_malloc (std_error_t * const error)
{
    remote_button_queue = xQueueCreate(REMOTE_BUTTON_QUEUE_SIZE, sizeof(node_remote_button_t));

    const bool are_queues_allocated = (remote_button_queue != NULL);

    light_level_timer = xTimerCreate("light_level", LIGHT_LEVEL_PERIOD_MIN, pdTRUE, NULL, node_light_level_timer);

    const bool are_timers_allocated = (light_level_timer != NULL);

    if ((are_queues_allocated != true) || (are_timers_allocated != true))
    {
        vQueueDelete(remote_button_queue);
        xTimerDelete(light_level_timer, 0U);

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
