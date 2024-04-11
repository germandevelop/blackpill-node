/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#include "board_B02.h"

#include <string.h>
#include <limits.h>
#include <math.h>
#include <assert.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

#include "board.i2c_1.h"

#include "devices/mcp23017_expander.h"
#include "devices/bmp280_sensor.h"
#include "devices/ssd1306_display.h"

#include "node_B02.h"

#include "logger.h"
#include "std_error/std_error.h"


#define RTOS_TASK_STACK_SIZE        1024U       // 1024 * 4 = 4096 bytes
#define RTOS_TASK_PRIORITY          1U          // 0 - lowest, 4 - highest
#define RTOS_TASK_NAME              "board_B02" // 16 - max length
#define RTOS_TIMER_TICKS_TO_WAIT    100U

#define DOOR_PIR_NOTIFICATION               (1 << 0)
#define FRONT_PIR_NOTIFICATION              (1 << 1)
#define VERANDA_PIR_NOTIFICATION            (1 << 2)
#define LIGHTNING_BLOCK_NOTIFICATION        (1 << 3)
#define LIGHTNING_UNBLOCK_NOTIFICATION      (1 << 4)
#define FRONT_LIGHT_NOTIFICATION            (1 << 5)
#define VERANDA_LIGHT_NOTIFICATION          (1 << 6)
#define LIGHT_STRIP_WHITE_NOTIFICATION      (1 << 7)
#define LIGHT_STRIP_GREEN_BLUE_NOTIFICATION (1 << 8)
#define LIGHT_STRIP_RED_NOTIFICATION        (1 << 9)
#define DISPLAY_NOTIFICATION                (1 << 10)
#define BUZZER_NOTIFICATION                 (1 << 11)
#define TEMPERATURE_SENSOR_NOTIFICATION     (1 << 12)
#define FRONT_PIR_POWER_NOTIFICATION        (1 << 13)
#define FRONT_PIR_CONFIG_NOTIFICATION       (1 << 14)
#define UPDATE_STATE_NOTIFICATION           (1 << 15)

#define I2C_TIMEOUT_MS (1U * 1000U) // 1 sec

#define PIR_HYSTERESIS_MS (1U * 1000U) // 1 sec

#define DEFAULT_ERROR_TEXT  "Board B02 error"
#define MALLOC_ERROR_TEXT   "Board B02 memory allocation error"

#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


static TaskHandle_t task;
static SemaphoreHandle_t node_mutex;
static TimerHandle_t temperature_timer;
static TimerHandle_t front_pir_timer;
static TimerHandle_t lightning_block_timer;
static TimerHandle_t display_timer;
static TimerHandle_t veranda_light_timer;
static TimerHandle_t front_light_timer;
static TimerHandle_t light_strip_white_timer;
static TimerHandle_t light_strip_green_blue_timer;
static TimerHandle_t light_strip_red_timer;
static TimerHandle_t buzzer_timer;

static board_extension_config_t config;

static node_B02_t *node;


static int board_B02_malloc (std_error_t * const error);
static void board_B02_task (void *parameters);

static void board_B02_lightning_block_timer (TimerHandle_t timer);
static void board_B02_veranda_light_timer (TimerHandle_t timer);
static void board_B02_front_light_timer (TimerHandle_t timer);
static void board_B02_light_strip_white_timer (TimerHandle_t timer);
static void board_B02_light_strip_green_blue_timer (TimerHandle_t timer);
static void board_B02_light_strip_red_timer (TimerHandle_t timer);
static void board_B02_buzzer_timer (TimerHandle_t timer);
static void board_B02_front_pir_timer (TimerHandle_t timer);

static void board_B02_draw_display (bool * const is_display_enabled, std_error_t * const error);
static void board_B02_draw_blue_display (node_B02_temperature_t const * const data, std_error_t * const error);
static void board_B02_display_timer (TimerHandle_t timer);

static void board_B02_read_temperature_data (std_error_t * const error);
static void board_B02_temperature_timer (TimerHandle_t timer);

static void board_B02_enable_display_power ();
static void board_B02_disable_display_power ();
static void board_B02_enable_veranda_light_power ();
static void board_B02_disable_veranda_light_power ();
static void board_B02_enable_front_light_power ();
static void board_B02_disable_front_light_power ();
static void board_B02_enable_light_strip_white_power ();
static void board_B02_disable_light_strip_white_power ();
static void board_B02_enable_light_strip_green_power ();
static void board_B02_disable_light_strip_green_power ();
static void board_B02_enable_light_strip_blue_power ();
static void board_B02_disable_light_strip_blue_power ();
static void board_B02_enable_light_strip_red_power ();
static void board_B02_disable_light_strip_red_power ();
static void board_B02_enable_buzzer_power ();
static void board_B02_disable_buzzer_power ();
static void board_B02_enable_front_pir_power ();
static void board_B02_disable_front_pir_power ();

static void board_B02_door_pir_ISR ();
static void board_B02_front_pir_ISR ();
static void board_B02_veranda_pir_ISR ();

static void board_B02_init_temperature_sensor ();

int board_B02_init (board_extension_config_t const * const init_config, std_error_t * const error)
{
    assert(init_config                              != NULL);
    assert(init_config->mcp23017_expander           != NULL);
    assert(init_config->storage                     != NULL);
    assert(init_config->update_status_led_callback  != NULL);
    assert(init_config->send_node_msg_callback      != NULL);

    config = *init_config;

    return board_B02_malloc(error);
}

void board_B02_is_remote_control_enabled (bool * const is_remote_control_enabled)
{
    *is_remote_control_enabled = false;

    return;
}

void board_B02_task (void *parameters)
{
    UNUSED(parameters);

    node_B02_init(node);
    board_B02_init_temperature_sensor();

    std_error_t error;
    std_error_init(&error);

    bool is_lightning_blocked               = false;
    bool is_front_light_enabled             = false;
    bool is_veranda_light_enabled           = false;
    bool is_light_strip_white_enabled       = false;
    bool is_light_strip_green_enabled       = false;
    size_t light_strip_green_blue_counter   = 0U;
    bool is_light_strip_red_enabled         = false;
    size_t light_strip_red_counter          = 0U;
    bool is_display_enabled                 = false;
    bool is_buzzer_enabled                  = false;
    bool is_front_pir_enabled               = false;

    xTimerChangePeriod(temperature_timer, pdMS_TO_TICKS(NODE_B02_TEMPERATURE_PERIOD_MS), RTOS_TIMER_TICKS_TO_WAIT);

    while (true)
    {
        uint32_t notification;
        xTaskNotifyWait(0U, ULONG_MAX, &notification, portMAX_DELAY);

        const uint32_t tick_count_ms = xTaskGetTickCount();

        if ((notification & DOOR_PIR_NOTIFICATION) != 0U)
        {
            LOG("Board B02 [door_pir] : movement\r\n");

            xSemaphoreTake(node_mutex, portMAX_DELAY);
            node_B02_process_door_movement(node, tick_count_ms);
            xSemaphoreGive(node_mutex);
        }

        if ((notification & FRONT_PIR_NOTIFICATION) != 0U)
        {
            LOG("Board B02 [front_pir] : movement\r\n");

            xSemaphoreTake(node_mutex, portMAX_DELAY);
            node_B02_process_front_movement(node, tick_count_ms);
            xSemaphoreGive(node_mutex);
        }

        if ((notification & VERANDA_PIR_NOTIFICATION) != 0U)
        {
            LOG("Board B02 [veranda_pir] : movement\r\n");

            xSemaphoreTake(node_mutex, portMAX_DELAY);
            node_B02_process_veranda_movement(node, tick_count_ms);
            xSemaphoreGive(node_mutex);
        }

        if ((notification & LIGHTNING_BLOCK_NOTIFICATION) != 0U)
        {
            if (is_lightning_blocked == false)
            {
                is_lightning_blocked = true;

                xTimerStop(front_light_timer, RTOS_TIMER_TICKS_TO_WAIT);
                xTimerStop(veranda_light_timer, RTOS_TIMER_TICKS_TO_WAIT);
                xTimerStop(light_strip_white_timer, RTOS_TIMER_TICKS_TO_WAIT);
                xTimerStop(light_strip_green_blue_timer, RTOS_TIMER_TICKS_TO_WAIT);
                xTimerStop(light_strip_red_timer, RTOS_TIMER_TICKS_TO_WAIT);
                xTimerStop(display_timer, RTOS_TIMER_TICKS_TO_WAIT);

                is_front_light_enabled          = false;
                is_veranda_light_enabled        = false;
                is_light_strip_white_enabled    = false;
                light_strip_green_blue_counter  = 0U;
                is_light_strip_red_enabled      = false;
                is_display_enabled              = false;

                board_B02_disable_front_light_power();
                board_B02_disable_veranda_light_power();
                board_B02_disable_light_strip_white_power();
                board_B02_disable_light_strip_green_power();
                board_B02_disable_light_strip_blue_power();
                board_B02_disable_light_strip_red_power();
                board_B02_disable_display_power();
            }
        }

        if ((notification & LIGHTNING_UNBLOCK_NOTIFICATION) != 0U)
        {
            is_lightning_blocked = false;
        }

        if ((notification & FRONT_LIGHT_NOTIFICATION) != 0U)
        {
            if (is_front_light_enabled == false)
            {
                is_front_light_enabled = true;
                board_B02_enable_front_light_power();

                xTimerChangePeriod(front_light_timer, pdMS_TO_TICKS(NODE_B02_LIGHT_DURATION_MS), RTOS_TIMER_TICKS_TO_WAIT);
            }
            else
            {
                is_front_light_enabled = false;
                board_B02_disable_front_light_power();
            }
        }

        if ((notification & VERANDA_LIGHT_NOTIFICATION) != 0U)
        {
            if (is_veranda_light_enabled == false)
            {
                is_veranda_light_enabled = true;
                board_B02_enable_veranda_light_power();

                xTimerChangePeriod(veranda_light_timer, pdMS_TO_TICKS(NODE_B02_LIGHT_DURATION_MS), RTOS_TIMER_TICKS_TO_WAIT);
            }
            else
            {
                is_veranda_light_enabled = false;
                board_B02_disable_veranda_light_power();
            }
        }

        if ((notification & LIGHT_STRIP_WHITE_NOTIFICATION) != 0U)
        {
            if (is_light_strip_white_enabled == false)
            {
                is_light_strip_white_enabled = true;
                board_B02_enable_light_strip_white_power();

                xTimerChangePeriod(light_strip_white_timer, pdMS_TO_TICKS(NODE_B02_LIGHT_DURATION_MS), RTOS_TIMER_TICKS_TO_WAIT);
            }
            else
            {
                is_light_strip_white_enabled = false;
                board_B02_disable_light_strip_white_power();
            }
        }

        if ((notification & LIGHT_STRIP_GREEN_BLUE_NOTIFICATION) != 0U)
        {
            if (light_strip_green_blue_counter > NODE_B02_LIGHT_DURATION_MS)
            {
                light_strip_green_blue_counter = 0U;
                board_B02_disable_light_strip_green_power();
                board_B02_disable_light_strip_blue_power();
            }
            else
            {
                ++light_strip_green_blue_counter;

                if (is_light_strip_green_enabled == false)
                {
                    is_light_strip_green_enabled = true;
                    board_B02_enable_light_strip_green_power();
                    board_B02_disable_light_strip_blue_power();

                    xTimerChangePeriod(light_strip_green_blue_timer, pdMS_TO_TICKS(2U * 1000U), RTOS_TIMER_TICKS_TO_WAIT);
                }
                else
                {
                    is_light_strip_green_enabled = false;
                    board_B02_enable_light_strip_blue_power();
                    board_B02_disable_light_strip_green_power();

                    xTimerChangePeriod(light_strip_green_blue_timer, pdMS_TO_TICKS(2U * 1000U), RTOS_TIMER_TICKS_TO_WAIT);
                }
            }
        }

        if ((notification & LIGHT_STRIP_RED_NOTIFICATION) != 0U)
        {
            if (is_light_strip_red_enabled == true)
            {
                const bool is_even = (light_strip_red_counter % 2U) == 0U;

                if (is_even == true)
                {
                    board_B02_enable_light_strip_red_power();

                    xTimerChangePeriod(light_strip_red_timer, pdMS_TO_TICKS(3U * 1000U), RTOS_TIMER_TICKS_TO_WAIT);
                }
                else
                {
                    board_B02_disable_light_strip_red_power();

                    xTimerChangePeriod(light_strip_red_timer, pdMS_TO_TICKS(1U * 1000U), RTOS_TIMER_TICKS_TO_WAIT);
                }

                ++light_strip_red_counter;
            }
            else
            {
                board_B02_disable_light_strip_red_power();
            }
        }

        if ((notification & DISPLAY_NOTIFICATION) != 0U)
        {
            board_B02_draw_display(&is_display_enabled, &error);
        }

        if ((notification & BUZZER_NOTIFICATION) != 0U)
        {
            if (is_buzzer_enabled == false)
            {
                is_buzzer_enabled = true;
                board_B02_enable_buzzer_power();

                xTimerChangePeriod(buzzer_timer, pdMS_TO_TICKS(NODE_B02_BUZZER_DURATION_MS), RTOS_TIMER_TICKS_TO_WAIT);
            }
            else
            {
                is_buzzer_enabled = false;
                board_B02_disable_buzzer_power();
            }
        }

        if ((notification & TEMPERATURE_SENSOR_NOTIFICATION) != 0U)
        {
            board_B02_read_temperature_data(&error);
        }

        if ((notification & FRONT_PIR_POWER_NOTIFICATION) != 0U)
        {
            if (is_front_light_enabled == false)
            {
                is_front_light_enabled = true;
                board_B02_enable_front_pir_power();

                xTimerChangePeriod(front_pir_timer, pdMS_TO_TICKS(3U * 1000U), RTOS_TIMER_TICKS_TO_WAIT);
            }
            else
            {
                is_front_light_enabled = false;
                // Disable EXTI
                board_B02_disable_front_pir_power();
            }
        }

        if ((notification & FRONT_PIR_CONFIG_NOTIFICATION) != 0U)
        {
            if (is_front_light_enabled == true)
            {
                // Enable EXTI
            }
        }

        // Update B02 states
        {
            node_B02_state_t node_state;

            xSemaphoreTake(node_mutex, portMAX_DELAY);
            node_B02_get_state(node, &node_state, tick_count_ms);
            xSemaphoreGive(node_mutex);

            // Send messages
            if (node_state.is_msg_to_send == true)
            {
                while (true)
                {
                    bool is_msg_valid;
                    node_msg_t send_msg;

                    xSemaphoreTake(node_mutex, portMAX_DELAY);
                    node_B02_get_msg(node, &send_msg, &is_msg_valid);
                    xSemaphoreGive(node_mutex);

                    if (is_msg_valid == false)
                    {
                        break;
                    }

                    if (config.send_node_msg_callback(&send_msg, &error) != STD_SUCCESS)
                    {
                        LOG("Board B02 [node] : %s\r\n", error.text);
                    }
                }
            }

            if (is_lightning_blocked == false)
            {
                // Update veranda light state
                if (node_state.is_veranda_light_on == true)
                {
                    if (is_veranda_light_enabled == false)
                    {
                        xTaskNotify(task, VERANDA_LIGHT_NOTIFICATION, eSetBits);
                    }
                }

                // Update front light state
                if (node_state.is_front_light_on == true)
                {
                    if (is_front_light_enabled == false)
                    {
                        xTaskNotify(task, FRONT_LIGHT_NOTIFICATION, eSetBits);
                    }
                }

                // Update light strip white state
                if (node_state.light_strip.is_white_on == true)
                {
                    if (is_light_strip_white_enabled == false)
                    {
                        xTaskNotify(task, LIGHT_STRIP_WHITE_NOTIFICATION, eSetBits);
                    }
                }

                // Update light strip green and blue state
                if (node_state.light_strip.is_blue_green_on == true)
                {
                    if (light_strip_green_blue_counter == 0U)
                    {
                        xTaskNotify(task, LIGHT_STRIP_GREEN_BLUE_NOTIFICATION, eSetBits);
                    }
                }

                // Update light strip red state
                if (node_state.light_strip.is_red_on == true)
                {
                    if (is_light_strip_red_enabled == false)
                    {
                        is_light_strip_red_enabled = true;

                        xTaskNotify(task, LIGHT_STRIP_RED_NOTIFICATION, eSetBits);
                    }
                }
                else
                {
                    is_light_strip_red_enabled = false;
                }

                // Update display state
                if (node_state.is_display_on == true)
                {
                    if (is_display_enabled == false)
                    {
                        xTaskNotify(task, DISPLAY_NOTIFICATION, eSetBits);
                    }
                }

                // Update status LED color
                config.update_status_led_callback(node_state.status_led_color);
            }

            // Update buzzer state
            if (node_state.is_buzzer_on == true)
            {
                if (is_buzzer_enabled == false)
                {
                    xTaskNotify(task, BUZZER_NOTIFICATION, eSetBits);
                }
            }

            // Update front pir state
            if (node_state.is_front_pir_on == true)
            {
                if (is_front_pir_enabled == false)
                {
                    xTaskNotify(task, LIGHT_STRIP_GREEN_BLUE_NOTIFICATION, eSetBits);
                }
            }
            else
            {
                if (is_front_pir_enabled == true)
                {
                    xTaskNotify(task, LIGHT_STRIP_GREEN_BLUE_NOTIFICATION, eSetBits);
                }
            }
        }

        LOG("Board B02 : loop\r\n");
    }

    return;
}

void board_B02_process_remote_button (board_remote_button_t remote_button)
{
    xSemaphoreTake(node_mutex, portMAX_DELAY);
    node_B02_process_remote_button(node, remote_button);
    xSemaphoreGive(node_mutex);

    xTaskNotify(task, UPDATE_STATE_NOTIFICATION, eSetBits);

    return;
}

void board_B02_process_photoresistor_data (photoresistor_data_t const * const data, uint32_t * const next_time_ms)
{
    assert(data         != NULL);
    assert(next_time_ms != NULL);

    const float gamma                   = 0.60F;        // Probably it does not work
    const float one_lux_resistance_Ohm  = 200000.0F;    // Probably it does not work

    const float lux = pow(10.0F, (log10(one_lux_resistance_Ohm / (float)(data->resistance_Ohm)) / gamma));  // Probably it does not work

    LOG("Board B02 [photoresistor] : luminosity = %.2f lux\r\n", lux);

    node_B02_luminosity_t luminosity;
    luminosity.lux      = round(lux);
    luminosity.is_valid = true;

    xSemaphoreTake(node_mutex, portMAX_DELAY);
    node_B02_process_luminosity(node, &luminosity, next_time_ms);
    xSemaphoreGive(node_mutex);

    xTaskNotify(task, UPDATE_STATE_NOTIFICATION, eSetBits);

    return;
}

void board_B02_process_node_msg (node_msg_t const * const rcv_msg)
{
    assert(rcv_msg != NULL);

    const uint32_t tick_count_ms = xTaskGetTickCount();

    xSemaphoreTake(node_mutex, portMAX_DELAY);
    node_B02_process_msg(node, rcv_msg, tick_count_ms);
    xSemaphoreGive(node_mutex);

    xTaskNotify(task, UPDATE_STATE_NOTIFICATION, eSetBits);

    return;
}

void board_B02_disable_lightning (uint32_t period_ms, bool * const is_lightning_disabled)
{
    assert(period_ms                != 0U);
    assert(is_lightning_disabled    != NULL);

    *is_lightning_disabled = true;

    xTimerChangePeriod(lightning_block_timer, pdMS_TO_TICKS(period_ms), RTOS_TIMER_TICKS_TO_WAIT);

    xTaskNotify(task, LIGHTNING_BLOCK_NOTIFICATION, eSetBits);

    return;
}

void board_B02_lightning_block_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    xTaskNotify(task, LIGHTNING_UNBLOCK_NOTIFICATION, eSetBits);

    return;
}

void board_B02_veranda_light_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    xTaskNotify(task, VERANDA_LIGHT_NOTIFICATION, eSetBits);

    return;
}

void board_B02_front_light_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    xTaskNotify(task, FRONT_LIGHT_NOTIFICATION, eSetBits);

    return;
}

void board_B02_light_strip_white_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    xTaskNotify(task, LIGHT_STRIP_WHITE_NOTIFICATION, eSetBits);

    return;
}

void board_B02_light_strip_green_blue_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    xTaskNotify(task, LIGHT_STRIP_GREEN_BLUE_NOTIFICATION, eSetBits);

    return;
}

void board_B02_light_strip_red_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    xTaskNotify(task, LIGHT_STRIP_RED_NOTIFICATION, eSetBits);

    return;
}

void board_B02_buzzer_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    xTaskNotify(task, BUZZER_NOTIFICATION, eSetBits);

    return;
}

void board_B02_front_pir_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    xTaskNotify(task, FRONT_PIR_CONFIG_NOTIFICATION, eSetBits);

    return;
}


void board_B02_draw_display (bool * const is_display_enabled, std_error_t * const error)
{
    typedef enum display_stage
    {
        ENABLE_POWER = 0,
        DRAW_DATA,
        DISABLE_POWER

    } display_stage_t;

    static display_stage_t stage = ENABLE_POWER;

    if (stage == ENABLE_POWER)
    {
        stage = DRAW_DATA;

        *is_display_enabled = true;
        board_B02_enable_display_power();

        xTimerChangePeriod(display_timer, pdMS_TO_TICKS(1U * 1000U), RTOS_TIMER_TICKS_TO_WAIT);
    }

    else if (stage == DRAW_DATA)
    {
        stage = DISABLE_POWER;

        node_B02_temperature_t data;
        uint32_t disable_time_ms;

        xSemaphoreTake(node_mutex, portMAX_DELAY);
        node_B02_get_display_data(node, &data, &disable_time_ms);
        xSemaphoreGive(node_mutex);

        board_B02_draw_blue_display(&data, error);

        xTimerChangePeriod(display_timer, pdMS_TO_TICKS(disable_time_ms), RTOS_TIMER_TICKS_TO_WAIT);
    }

    else if (stage == DISABLE_POWER)
    {
        stage = ENABLE_POWER;

        *is_display_enabled = false;
        board_B02_disable_display_power();
    }

    return;
}

void board_B02_draw_blue_display (node_B02_temperature_t const * const data, std_error_t * const error)
{
    assert(data != NULL);

    LOG("Board B02 [blue_display] : draw\r\n");

    // Prepare text to draw
    const uint8_t temp_text[] = {  0xD2, 0xC5, 0xCC, 0xCF, 0xC5, 0xD0, 0xC0, 0xD2, 0xD3, 0xD0, 0xC0 };
    const uint8_t error_text[] = {  0xCE, 0xF8, 0xE8, 0xE1, 0xEA, 0xE0 };

    char temp_value[16] = { '\0' };
    sprintf(temp_value, "%+.1f C", data->temperature_C);

    const uint8_t x_text_min = 2U, y_text_min = 6U;
    const uint8_t x_text_max = 10U, y_text_max = 20U;
    const uint8_t y_value_shift = 20U;

    static uint8_t x_current = x_text_max;
    static uint8_t y_current = y_text_max;

    x_current += 2U;
    y_current += 2U;

    if (x_current > x_text_max)
    {
        x_current = x_text_min;
    }

    if (y_current > y_text_max)
    {
        y_current = y_text_min;
    }

    // Draw the text
    uint8_t ssd1306_pixel_buffer[SSD1306_DISPLAY_PIXEL_BUFFER_SIZE];

    ssd1306_display_config_t config;
    config.write_i2c_callback       = board_i2c_1_write;
    config.write_i2c_dma_callback   = NULL;
    config.i2c_timeout_ms           = I2C_TIMEOUT_MS;
    config.pixel_buffer             = ssd1306_pixel_buffer;
    config.device_address           = SSD1306_DISPLAY_ADDRESS_2;

    ssd1306_display_t ssd1306_display;

    if (ssd1306_display_init(&ssd1306_display, &config, error) != STD_SUCCESS)
    {
        LOG("Board B02 [blue_display] : %s\r\n", error->text);

        return;
    }

    ssd1306_display_reset_buffer(&ssd1306_display);

    if (ssd1306_display_update_full_screen(&ssd1306_display, error) != STD_SUCCESS)
    {
        LOG("Board B02 [blue_display] : %s\r\n", error->text);
    }

    if (data->is_valid == true)
    {
        uint8_t X_shift;
        ssd1306_display_draw_text_10x16(&ssd1306_display, (const char*)temp_text, ARRAY_SIZE(temp_text), x_current, y_current, &X_shift);
        ssd1306_display_draw_text_16x26(&ssd1306_display, temp_value, strlen(temp_value), x_current, y_current + y_value_shift, &X_shift);
    }
    else
    {
        uint8_t X_shift;
        ssd1306_display_draw_text_10x16(&ssd1306_display, (const char*)error_text, ARRAY_SIZE(error_text), x_current, y_current, &X_shift);
    }

    if (ssd1306_display_update_full_screen(&ssd1306_display, error) != STD_SUCCESS)
    {
        LOG("Board B02 [blue_display] : %s\r\n", error->text);
    }

    return;
}

void board_B02_display_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    xTaskNotify(task, DISPLAY_NOTIFICATION, eSetBits);

    return;
}


void board_B02_read_temperature_data (std_error_t * const error)
{
    LOG("Board B02 [bmp280] : read\r\n");

    node_B02_temperature_t temperature;
    temperature.is_valid = false;

    bmp280_sensor_data_t data;

    if (bmp280_sensor_read_data(&data, error) != STD_FAILURE)
    {
        temperature.pressure_hPa    = data.pressure_hPa;
        temperature.temperature_C   = data.temperature_C;
        temperature.is_valid        = true;

        LOG("Board B02 [bmp280] : temperature = %.2f C\r\n", data.temperature_C);
        LOG("Board B02 [bmp280] : pressure = %.1f hPa\r\n", data.pressure_hPa);
    }
    else
    {
        LOG("Board B02 [bmp280] : %s\r\n", error->text);
    }

    uint32_t next_time_ms;

    xSemaphoreTake(node_mutex, portMAX_DELAY);
    node_B02_process_temperature(node, &temperature, &next_time_ms);
    xSemaphoreGive(node_mutex);

    xTimerChangePeriod(temperature_timer, pdMS_TO_TICKS(next_time_ms), RTOS_TIMER_TICKS_TO_WAIT);

    return;
}

void board_B02_temperature_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    xTaskNotify(task, TEMPERATURE_SENSOR_NOTIFICATION, eSetBits);

    return;
}


void board_B02_enable_display_power ()
{
    LOG("Board B02 [display] : enable power\r\n");

    return;
}

void board_B02_disable_display_power ()
{
    LOG("Board B02 [display] : disable power\r\n");

    return;
}

void board_B02_enable_veranda_light_power ()
{
    LOG("Board B02 [veranda_light] : enable power\r\n");

    return;
}

void board_B02_disable_veranda_light_power ()
{
    LOG("Board B02 [veranda_light] : disable power\r\n");

    return;
}

void board_B02_enable_front_light_power ()
{
    LOG("Board B02 [front_light] : enable power\r\n");

    return;
}

void board_B02_disable_front_light_power ()
{
    LOG("Board B02 [front_light] : disable power\r\n");

    return;
}

void board_B02_enable_light_strip_white_power ()
{
    LOG("Board B02 [strip_white] : enable power\r\n");

    return;
}

void board_B02_disable_light_strip_white_power ()
{
    LOG("Board B02 [strip_white] : disable power\r\n");

    return;
}

void board_B02_enable_light_strip_green_power ()
{
    LOG("Board B02 [strip_green] : enable power\r\n");

    return;
}

void board_B02_disable_light_strip_green_power ()
{
    LOG("Board B02 [strip_green] : disable power\r\n");

    return;
}

void board_B02_enable_light_strip_blue_power ()
{
    LOG("Board B02 [strip_blue] : enable power\r\n");

    return;
}

void board_B02_disable_light_strip_blue_power ()
{
    LOG("Board B02 [strip_blue] : disable power\r\n");

    return;
}

void board_B02_enable_light_strip_red_power ()
{
    LOG("Board B02 [strip_red] : enable power\r\n");

    return;
}

void board_B02_disable_light_strip_red_power ()
{
    LOG("Board B02 [strip_red] : disable power\r\n");

    return;
}

void board_B02_enable_buzzer_power ()
{
    LOG("Board B02 [buzzer] : enable power\r\n");

    return;
}

void board_B02_disable_buzzer_power ()
{
    LOG("Board B02 [buzzer] : disable power\r\n");

    return;
}

void board_B02_enable_front_pir_power ()
{
    LOG("Board B02 [front_pir] : enable power\r\n");

    return;
}

void board_B02_disable_front_pir_power ()
{
    LOG("Board B02 [front_pir] : disable power\r\n");

    return;
}


void board_B02_door_pir_ISR ()
{
    static TickType_t last_tick_count_ms = 0U;

    const TickType_t tick_count_ms = xTaskGetTickCountFromISR();

    if ((tick_count_ms - last_tick_count_ms) > PIR_HYSTERESIS_MS)
    {
        last_tick_count_ms = tick_count_ms;

        BaseType_t is_higher_priority_task_woken;
        xTaskNotifyFromISR(task, DOOR_PIR_NOTIFICATION, eSetBits, &is_higher_priority_task_woken);

        portYIELD_FROM_ISR(is_higher_priority_task_woken);
    }

    return;
}

void board_B02_front_pir_ISR ()
{
    static TickType_t last_tick_count_ms = 0U;

    const TickType_t tick_count_ms = xTaskGetTickCountFromISR();

    if ((tick_count_ms - last_tick_count_ms) > PIR_HYSTERESIS_MS)
    {
        last_tick_count_ms = tick_count_ms;

        BaseType_t is_higher_priority_task_woken;
        xTaskNotifyFromISR(task, FRONT_PIR_NOTIFICATION, eSetBits, &is_higher_priority_task_woken);

        portYIELD_FROM_ISR(is_higher_priority_task_woken);
    }

    return;
}

void board_B02_veranda_pir_ISR ()
{
    static TickType_t last_tick_count_ms = 0U;

    const TickType_t tick_count_ms = xTaskGetTickCountFromISR();

    if ((tick_count_ms - last_tick_count_ms) > PIR_HYSTERESIS_MS)
    {
        last_tick_count_ms = tick_count_ms;

        BaseType_t is_higher_priority_task_woken;
        xTaskNotifyFromISR(task, VERANDA_PIR_NOTIFICATION, eSetBits, &is_higher_priority_task_woken);

        portYIELD_FROM_ISR(is_higher_priority_task_woken);
    }

    return;
}


void board_B02_init_temperature_sensor ()
{
    std_error_t error;
    std_error_init(&error);

    LOG("Board B02 [bmp280] : init\r\n");

    bmp280_sensor_config_t config;
    config.write_i2c_callback   = board_i2c_1_write_register;
    config.read_i2c_callback    = board_i2c_1_read_register;
    config.i2c_timeout_ms       = I2C_TIMEOUT_MS; 
    config.delay_callback       = vTaskDelay;

    if (bmp280_sensor_init(&config, &error) != STD_SUCCESS)
    {
        LOG("Board B02 [bmp280] : %s\r\n", error.text);
    }

    return;
}


int board_B02_malloc (std_error_t * const error)
{
    node = (node_B02_t*)pvPortMalloc(sizeof(node_B02_t));

    const bool are_buffers_allocated = (node != NULL);

    node_mutex = xSemaphoreCreateMutex();

    const bool are_semaphores_allocated = (node_mutex != NULL);

    temperature_timer               = xTimerCreate("temperature", pdMS_TO_TICKS(1000U), pdFALSE, NULL, board_B02_temperature_timer);
    lightning_block_timer           = xTimerCreate("lightning_block", pdMS_TO_TICKS(1000U), pdFALSE, NULL, board_B02_lightning_block_timer);
    display_timer                   = xTimerCreate("display", pdMS_TO_TICKS(1000U), pdFALSE, NULL, board_B02_display_timer);
    front_pir_timer                 = xTimerCreate("front_pir", pdMS_TO_TICKS(1000U), pdFALSE, NULL, board_B02_front_pir_timer);
    veranda_light_timer             = xTimerCreate("veranda_light", pdMS_TO_TICKS(1000U), pdFALSE, NULL, board_B02_veranda_light_timer);
    front_light_timer               = xTimerCreate("front_light", pdMS_TO_TICKS(1000U), pdFALSE, NULL, board_B02_front_light_timer);
    light_strip_white_timer         = xTimerCreate("strip_white", pdMS_TO_TICKS(1000U), pdFALSE, NULL, board_B02_light_strip_white_timer);
    light_strip_green_blue_timer    = xTimerCreate("strip_green_blue", pdMS_TO_TICKS(1000U), pdFALSE, NULL, board_B02_light_strip_green_blue_timer);
    light_strip_red_timer           = xTimerCreate("strip_red", pdMS_TO_TICKS(1000U), pdFALSE, NULL, board_B02_light_strip_red_timer);
    buzzer_timer                    = xTimerCreate("buzzer", pdMS_TO_TICKS(1000U), pdFALSE, NULL, board_B02_buzzer_timer);

    const bool are_timers_allocated = (temperature_timer != NULL) && (lightning_block_timer != NULL) && (display_timer != NULL) &&
    (front_pir_timer != NULL) && (veranda_light_timer != NULL) && (front_light_timer != NULL) && (light_strip_white_timer != NULL) &&
    (light_strip_green_blue_timer != NULL) && (light_strip_red_timer != NULL) && (buzzer_timer != NULL);

    if ((are_buffers_allocated != true) || (are_semaphores_allocated != true) || (are_timers_allocated != true))
    {
        vPortFree((void*)node);
        vSemaphoreDelete(node_mutex);
        xTimerDelete(temperature_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(lightning_block_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(display_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(front_pir_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(veranda_light_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(front_light_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(light_strip_white_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(light_strip_green_blue_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(light_strip_red_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(buzzer_timer, RTOS_TIMER_TICKS_TO_WAIT);

        std_error_catch_custom(error, STD_FAILURE, MALLOC_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    BaseType_t exit_code = xTaskCreate(board_B02_task, RTOS_TASK_NAME, RTOS_TASK_STACK_SIZE, NULL, RTOS_TASK_PRIORITY, &task);

    if (exit_code != pdPASS)
    {
        std_error_catch_custom(error, (int)exit_code, MALLOC_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}
