/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#include "board_T01.h"

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
#include "devices/bme280_sensor.h"
#include "devices/ssd1306_display.h"

#include "node_T01.h"

#include "logger.h"
#include "std_error/std_error.h"


#define RTOS_TASK_STACK_SIZE        1024U       // 1024 * 4 = 4096 bytes
#define RTOS_TASK_PRIORITY          1U          // 0 - lowest, 4 - highest
#define RTOS_TASK_NAME              "board_T01" // 16 - max length
#define RTOS_TIMER_TICKS_TO_WAIT    100U

#define PIR_NOTIFICATION                (1 << 0)
#define LIGHTNING_BLOCK_NOTIFICATION    (1 << 1)
#define LIGHTNING_UNBLOCK_NOTIFICATION  (1 << 2)
#define LIGHT_NOTIFICATION              (1 << 3)
#define WARNING_LED_NOTIFICATION        (1 << 4)
#define DISPLAY_NOTIFICATION            (1 << 5)
#define HUMIDITY_SENSOR_NOTIFICATION    (1 << 6)
#define REED_SWITCH_NOTIFICATION        (1 << 7)
#define UPDATE_STATE_NOTIFICATION       (1 << 15)

#define I2C_TIMEOUT_MS (1U * 1000U) // 1 sec

#define PIR_HYSTERESIS_MS (1U * 1000U) // 1 sec

#define DEFAULT_ERROR_TEXT  "Board T01 error"
#define MALLOC_ERROR_TEXT   "Board T01 memory allocation error"

#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


static TaskHandle_t task;
static SemaphoreHandle_t node_mutex;
static TimerHandle_t humidity_timer;
static TimerHandle_t reed_switch_timer;
static TimerHandle_t lightning_block_timer;
static TimerHandle_t light_timer;
static TimerHandle_t display_timer;
static TimerHandle_t warning_led_timer;

static board_extension_config_t config;

static node_T01_t *node;


static int board_T01_malloc (std_error_t * const error);
static void board_T01_task (void *parameters);

static void board_T01_lightning_block_timer (TimerHandle_t timer);
static void board_T01_light_timer (TimerHandle_t timer);
static void board_T01_warning_led_timer (TimerHandle_t timer);

static void board_T01_draw_display (bool * const is_display_enabled, std_error_t * const error);
static void board_T01_draw_blue_display (node_T01_humidity_t const * const data, std_error_t * const error);
static void board_T01_draw_yellow_display (node_T01_humidity_t const * const data, std_error_t * const error);
static void board_T01_display_timer (TimerHandle_t timer);

static void board_T01_read_humidity_data (std_error_t * const error);
static void board_T01_humidity_timer (TimerHandle_t timer);

static void board_T01_read_reed_switch ();
static void board_T01_reed_switch_timer (TimerHandle_t timer);

static void board_T01_enable_display_power ();
static void board_T01_disable_display_power ();
static void board_T01_enable_light_power ();
static void board_T01_disable_light_power ();
static void board_T01_enable_warning_led_power ();
static void board_T01_disable_warning_led_power ();

static void board_T01_pir_ISR ();

static void board_B02_init_humidity_sensor ();

int board_T01_init (board_extension_config_t const * const init_config, std_error_t * const error)
{
    assert(init_config                              != NULL);
    assert(init_config->mcp23017_expander           != NULL);
    assert(init_config->storage                     != NULL);
    assert(init_config->update_status_led_callback  != NULL);
    assert(init_config->send_node_msg_callback      != NULL);

    config = *init_config;

    return board_T01_malloc(error);
}

void board_T01_is_remote_control_enabled (bool * const is_remote_control_enabled)
{
    *is_remote_control_enabled = false;

    return;
}

void board_T01_task (void *parameters)
{
    UNUSED(parameters);

    node_T01_init(node);
    board_B02_init_humidity_sensor();

    std_error_t error;
    std_error_init(&error);

    bool is_lightning_blocked   = false;
    bool is_light_enabled       = false;
    bool is_warning_led_enabled = false;
    size_t warning_led_counter  = 0U;
    bool is_display_enabled     = false;

    xTimerChangePeriod(humidity_timer, pdMS_TO_TICKS(NODE_T01_HUMIDITY_PERIOD_MS), RTOS_TIMER_TICKS_TO_WAIT);
    xTimerChangePeriod(reed_switch_timer, pdMS_TO_TICKS(NODE_T01_DOOR_STATE_PERIOD_MS), RTOS_TIMER_TICKS_TO_WAIT);

    while (true)
    {
        uint32_t notification;
        xTaskNotifyWait(0U, ULONG_MAX, &notification, portMAX_DELAY);

        const uint32_t tick_count_ms = xTaskGetTickCount();

        if ((notification & PIR_NOTIFICATION) != 0U)
        {
            LOG("Board T01 [pir] : movement\r\n");

            xSemaphoreTake(node_mutex, portMAX_DELAY);
            node_T01_process_movement(node, tick_count_ms);
            xSemaphoreGive(node_mutex);
        }

        if ((notification & LIGHTNING_BLOCK_NOTIFICATION) != 0U)
        {
            if (is_lightning_blocked == false)
            {
                is_lightning_blocked = true;

                xTimerStop(light_timer, RTOS_TIMER_TICKS_TO_WAIT);
                xTimerStop(warning_led_timer, RTOS_TIMER_TICKS_TO_WAIT);
                xTimerStop(display_timer, RTOS_TIMER_TICKS_TO_WAIT);

                is_light_enabled        = false;
                is_warning_led_enabled  = false;
                is_display_enabled      = false;

                board_T01_disable_light_power();
                board_T01_disable_warning_led_power();
                board_T01_disable_display_power();
            }
        }

        if ((notification & LIGHTNING_UNBLOCK_NOTIFICATION) != 0U)
        {
            is_lightning_blocked = false;
        }

        if ((notification & LIGHT_NOTIFICATION) != 0U)
        {
            if (is_light_enabled == false)
            {
                is_light_enabled = true;
                board_T01_enable_light_power();

                xTimerChangePeriod(light_timer, pdMS_TO_TICKS(NODE_T01_LIGHT_DURATION_MS), RTOS_TIMER_TICKS_TO_WAIT);
            }
            else
            {
                is_light_enabled = false;
                board_T01_disable_light_power();
            }
        }

        if ((notification & WARNING_LED_NOTIFICATION) != 0U)
        {
            if (is_warning_led_enabled == true)
            {
                const bool is_even = (warning_led_counter % 2U) == 0U;

                if (is_even == true)
                {
                    board_T01_enable_warning_led_power();

                    xTimerChangePeriod(warning_led_timer, pdMS_TO_TICKS(3U * 1000U), RTOS_TIMER_TICKS_TO_WAIT);
                }
                else
                {
                    board_T01_disable_warning_led_power();

                    xTimerChangePeriod(warning_led_timer, pdMS_TO_TICKS(1U * 1000U), RTOS_TIMER_TICKS_TO_WAIT);
                }

                ++warning_led_counter;
            }
            else
            {
                board_T01_disable_warning_led_power();
            }
        }

        if ((notification & DISPLAY_NOTIFICATION) != 0U)
        {
            board_T01_draw_display(&is_display_enabled, &error);
        }

        if ((notification & HUMIDITY_SENSOR_NOTIFICATION) != 0U)
        {
            board_T01_read_humidity_data(&error);
        }

        if ((notification & REED_SWITCH_NOTIFICATION) != 0U)
        {
            board_T01_read_reed_switch();
        }

        // Update T01 states
        {
            node_T01_state_t node_state;

            xSemaphoreTake(node_mutex, portMAX_DELAY);
            node_T01_get_state(node, &node_state, tick_count_ms);
            xSemaphoreGive(node_mutex);

            // Send messages
            if (node_state.is_msg_to_send == true)
            {
                while (true)
                {
                    bool is_msg_valid;
                    node_msg_t send_msg;

                    xSemaphoreTake(node_mutex, portMAX_DELAY);
                    node_T01_get_msg(node, &send_msg, &is_msg_valid);
                    xSemaphoreGive(node_mutex);

                    if (is_msg_valid == false)
                    {
                        break;
                    }

                    if (config.send_node_msg_callback(&send_msg, &error) != STD_SUCCESS)
                    {
                        LOG("Board T01 [node] : %s\r\n", error.text);
                    }
                }
            }

            if (is_lightning_blocked == false)
            {
                // Update light state
                if (node_state.is_light_on == true)
                {
                    if (is_light_enabled == false)
                    {
                        xTaskNotify(task, LIGHT_NOTIFICATION, eSetBits);
                    }
                }

                // Update warning LED state
                if (node_state.is_warning_led_on == true)
                {
                    if (is_warning_led_enabled == false)
                    {
                        is_warning_led_enabled = true;

                        xTaskNotify(task, WARNING_LED_NOTIFICATION, eSetBits);
                    }
                }
                else
                {
                    is_warning_led_enabled = false;
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
        }

        LOG("Board T01 : loop\r\n");
    }

    return;
}


void board_T01_process_remote_button (board_remote_button_t remote_button)
{
    xSemaphoreTake(node_mutex, portMAX_DELAY);
    node_T01_process_remote_button(node, remote_button);
    xSemaphoreGive(node_mutex);

    xTaskNotify(task, UPDATE_STATE_NOTIFICATION, eSetBits);

    return;
}

void board_T01_process_photoresistor_data (photoresistor_data_t const * const data, uint32_t * const next_time_ms)
{
    assert(data         != NULL);
    assert(next_time_ms != NULL);

    const float gamma                   = 0.60F;        // Probably it does not work
    const float one_lux_resistance_Ohm  = 200000.0F;    // Probably it does not work

    const float lux = pow(10.0F, (log10(one_lux_resistance_Ohm / (float)(data->resistance_Ohm)) / gamma));  // Probably it does not work

    LOG("Board T01 [photoresistor] : luminosity = %.2f lux\r\n", lux);

    node_T01_luminosity_t luminosity;
    luminosity.lux      = round(lux);
    luminosity.is_valid = true;

    xSemaphoreTake(node_mutex, portMAX_DELAY);
    node_T01_process_luminosity(node, &luminosity, next_time_ms);
    xSemaphoreGive(node_mutex);

    xTaskNotify(task, UPDATE_STATE_NOTIFICATION, eSetBits);

    return;
}

void board_T01_process_node_msg (node_msg_t const * const rcv_msg)
{
    assert(rcv_msg != NULL);

    const uint32_t tick_count_ms = xTaskGetTickCount();

    xSemaphoreTake(node_mutex, portMAX_DELAY);
    node_T01_process_msg(node, rcv_msg, tick_count_ms);
    xSemaphoreGive(node_mutex);

    xTaskNotify(task, UPDATE_STATE_NOTIFICATION, eSetBits);

    return;
}

void board_T01_disable_lightning (uint32_t period_ms, bool * const is_lightning_disabled)
{
    assert(period_ms                != 0U);
    assert(is_lightning_disabled    != NULL);

    *is_lightning_disabled = true;

    xTimerChangePeriod(lightning_block_timer, pdMS_TO_TICKS(period_ms), RTOS_TIMER_TICKS_TO_WAIT);

    xTaskNotify(task, LIGHTNING_BLOCK_NOTIFICATION, eSetBits);

    return;
}

void board_T01_lightning_block_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    xTaskNotify(task, LIGHTNING_UNBLOCK_NOTIFICATION, eSetBits);

    return;
}

void board_T01_light_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    xTaskNotify(task, LIGHT_NOTIFICATION, eSetBits);

    return;
}

void board_T01_warning_led_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    xTaskNotify(task, WARNING_LED_NOTIFICATION, eSetBits);

    return;
}

void board_T01_draw_display (bool * const is_display_enabled, std_error_t * const error)
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
        board_T01_enable_display_power();

        xTimerChangePeriod(display_timer, pdMS_TO_TICKS(1U * 1000U), RTOS_TIMER_TICKS_TO_WAIT);
    }

    else if (stage == DRAW_DATA)
    {
        stage = DISABLE_POWER;

        node_T01_humidity_t data;
        uint32_t disable_time_ms;

        xSemaphoreTake(node_mutex, portMAX_DELAY);
        node_T01_get_display_data(node, &data, &disable_time_ms);
        xSemaphoreGive(node_mutex);

        board_T01_draw_yellow_display(&data, error);
        board_T01_draw_blue_display(&data, error);

        xTimerChangePeriod(display_timer, pdMS_TO_TICKS(disable_time_ms), RTOS_TIMER_TICKS_TO_WAIT);
    }

    else if (stage == DISABLE_POWER)
    {
        stage = ENABLE_POWER;

        *is_display_enabled = false;
        board_T01_disable_display_power();
    }

    return;
}

void board_T01_draw_blue_display (node_T01_humidity_t const * const data, std_error_t * const error)
{
    LOG("Board T01 [display] : draw blue\r\n");

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

    ssd1306_display_config_t display_config;
    display_config.lock_i2c_callback    = config.lock_i2c_1_callback;
    display_config.unlock_i2c_callback  = config.unlock_i2c_1_callback;
    display_config.write_i2c_callback   = board_i2c_1_write;
    display_config.i2c_timeout_ms       = I2C_TIMEOUT_MS;
    display_config.pixel_buffer         = ssd1306_pixel_buffer;
    display_config.device_address       = SSD1306_DISPLAY_ADDRESS_2;

    ssd1306_display_t ssd1306_display;

    if (ssd1306_display_init(&ssd1306_display, &display_config, error) != STD_SUCCESS)
    {
        LOG("Board T01 [display] : blue = %s\r\n", error->text);

        return;
    }

    ssd1306_display_reset_buffer(&ssd1306_display);

    if (ssd1306_display_update_full_screen(&ssd1306_display, error) != STD_SUCCESS)
    {
        LOG("Board T01 [display] : blue = %s\r\n", error->text);
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
        LOG("Board T01 [display] : blue = %s\r\n", error->text);
    }

    return;
}

void board_T01_draw_yellow_display (node_T01_humidity_t const * const data, std_error_t * const error)
{
    //const uint8_t hum_text[] = {  0xE2, 0xEB, 0xE0, 0xE6, 0xED, 0xEE, 0xF1, 0xF2, 0xFC };
    //const uint8_t press_text[] = {  0xE4, 0xE0, 0xE2, 0xEB, 0xE5, 0xED, 0xE8, 0xE5 };
    //const uint8_t press_text[] = {  0xE3, 0xCF, 0xC0 };

    LOG("Board T01 [display] : draw yellow\r\n");

    // Prepare text to draw
    const uint8_t error_text[] = {  0xCE, 0xF8, 0xE8, 0xE1, 0xEA, 0xE0 };

    char hum_value[16] = { '\0' };
    sprintf(hum_value, "%.1f %%", data->humidity_pct);

    char press_value[16] = { '\0' };
    sprintf(press_value, "%.0f MM", data->pressure_hPa);

    const uint8_t x_hum_min = 2U, y_hum_min = 8U;
    const uint8_t x_hum_max = 32U, y_hum_max = 16U;
    const uint8_t y_press_shift = 28U;

    static uint8_t x_current = x_hum_max;
    static uint8_t y_current = y_hum_max;

    x_current += 2U;
    y_current += 2U;

    if (x_current > x_hum_max)
    {
        x_current = x_hum_min;
    }

    if (y_current > y_hum_max)
    {
        y_current = y_hum_min;
    }

    // Draw the text
    uint8_t ssd1306_pixel_buffer[SSD1306_DISPLAY_PIXEL_BUFFER_SIZE];

    ssd1306_display_config_t display_config;
    display_config.lock_i2c_callback    = config.lock_i2c_1_callback;
    display_config.unlock_i2c_callback  = config.unlock_i2c_1_callback;
    display_config.write_i2c_callback   = board_i2c_1_write;
    display_config.i2c_timeout_ms       = I2C_TIMEOUT_MS;
    display_config.pixel_buffer         = ssd1306_pixel_buffer;
    display_config.device_address       = SSD1306_DISPLAY_ADDRESS_1;

    ssd1306_display_t ssd1306_display;

    if (ssd1306_display_init(&ssd1306_display, &display_config, error) != STD_SUCCESS)
    {
        LOG("Board T01 [display] : yellow = %s\r\n", error->text);

        return;
    }

    ssd1306_display_reset_buffer(&ssd1306_display);

    if (ssd1306_display_update_full_screen(&ssd1306_display, error) != STD_SUCCESS)
    {
        LOG("Board T01 [display] : yellow = %s\r\n", error->text);
    }

    if (data->is_valid == true)
    {
        uint8_t X_shift;
        ssd1306_display_draw_text_16x26(&ssd1306_display, hum_value, strlen(hum_value), x_current, y_current, &X_shift);
        ssd1306_display_draw_text_16x26(&ssd1306_display, press_value, strlen(press_value), x_current, y_current + y_press_shift, &X_shift);
    }
    else
    {
        uint8_t X_shift;
        ssd1306_display_draw_text_10x16(&ssd1306_display, (const char*)error_text, ARRAY_SIZE(error_text), x_current, y_current, &X_shift);
    }

    if (ssd1306_display_update_full_screen(&ssd1306_display, error) != STD_SUCCESS)
    {
        LOG("Board T01 [display] : yellow = %s\r\n", error->text);
    }

    return;
}

void board_T01_display_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    xTaskNotify(task, DISPLAY_NOTIFICATION, eSetBits);

    return;
}


void board_T01_read_humidity_data (std_error_t * const error)
{
    LOG("Board T01 [bme280] : read\r\n");

    node_T01_humidity_t humidity;
    humidity.is_valid = false;
    
    bme280_sensor_data_t data;

    if (bme280_sensor_read_data(&data, error) != STD_FAILURE)
    {
        humidity.humidity_pct   = data.humidity_pct;
        humidity.pressure_hPa   = data.pressure_hPa;
        humidity.temperature_C  = data.temperature_C;
        humidity.is_valid       = true;

        LOG("Board T01 [bme280] : humidity = %.1f %%\r\n", data.humidity_pct);
        LOG("Board T01 [bme280] : temperature = %.2f C\r\n", data.temperature_C);
        LOG("Board T01 [bme280] : pressure = %.1f hPa\r\n", data.pressure_hPa);
    }
    else
    {
        LOG("Board T01 [bme280] : %s\r\n", error->text);
    }

    uint32_t next_time_ms;

    xSemaphoreTake(node_mutex, portMAX_DELAY);
    node_T01_process_humidity(node, &humidity, &next_time_ms);
    xSemaphoreGive(node_mutex);

    xTimerChangePeriod(humidity_timer, pdMS_TO_TICKS(next_time_ms), RTOS_TIMER_TICKS_TO_WAIT);

    return;
}

void board_T01_humidity_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    xTaskNotify(task, HUMIDITY_SENSOR_NOTIFICATION, eSetBits);

    return;
}


void board_T01_read_reed_switch ()
{
    LOG("Board T01 [reed_switch] : read\r\n");

    // **************************************
    // Read state
    bool is_reed_switch_open = false;
    // **************************************

    bool is_door_open = false;

    if (is_reed_switch_open == true)
    {
        is_door_open = true;
    }

    uint32_t next_time_ms;

    xSemaphoreTake(node_mutex, portMAX_DELAY);
    node_T01_process_door_state(node, is_door_open, &next_time_ms);
    xSemaphoreGive(node_mutex);

    xTimerChangePeriod(reed_switch_timer, pdMS_TO_TICKS(next_time_ms), RTOS_TIMER_TICKS_TO_WAIT);

    return;
}

void board_T01_reed_switch_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    xTaskNotify(task, REED_SWITCH_NOTIFICATION, eSetBits);

    return;
}


void board_T01_enable_display_power ()
{
    LOG("Board T01 [display] : enable power\r\n");

    return;
}

void board_T01_disable_display_power ()
{
    LOG("Board T01 [display] : disable power\r\n");

    return;
}

void board_T01_enable_light_power ()
{
    LOG("Board T01 [light] : enable power\r\n");

    return;
}

void board_T01_disable_light_power ()
{
    LOG("Board T01 [light] : disable power\r\n");

    return;
}

void board_T01_enable_warning_led_power ()
{
    LOG("Board T01 [warning_led] : enable power\r\n");

    return;
}

void board_T01_disable_warning_led_power ()
{
    LOG("Board T01 [warning_led] : disable power\r\n");

    return;
}


void board_T01_pir_ISR ()
{
    static TickType_t last_tick_count_ms = 0U;

    const TickType_t tick_count_ms = xTaskGetTickCountFromISR();

    if ((tick_count_ms - last_tick_count_ms) > PIR_HYSTERESIS_MS)
    {
        last_tick_count_ms = tick_count_ms;

        BaseType_t is_higher_priority_task_woken;
        xTaskNotifyFromISR(task, PIR_NOTIFICATION, eSetBits, &is_higher_priority_task_woken);

        portYIELD_FROM_ISR(is_higher_priority_task_woken);
    }

    return;
}


void board_B02_init_humidity_sensor ()
{
    LOG("Board T01 [bme280] : init\r\n");

    std_error_t error;
    std_error_init(&error);

    bme280_sensor_config_t sensor_config;
    sensor_config.lock_i2c_callback     = config.lock_i2c_1_callback;
    sensor_config.unlock_i2c_callback   = config.unlock_i2c_1_callback;
    sensor_config.write_i2c_callback    = board_i2c_1_write_register;
    sensor_config.read_i2c_callback     = board_i2c_1_read_register;
    sensor_config.i2c_timeout_ms        = I2C_TIMEOUT_MS;
    sensor_config.delay_callback        = vTaskDelay;

    if (bme280_sensor_init(&sensor_config, &error) != STD_SUCCESS)
    {
        LOG("Board T01 [bme280] : %s\r\n", error.text);
    }

    return;
}


int board_T01_malloc (std_error_t * const error)
{
    node = (node_T01_t*)pvPortMalloc(sizeof(node_T01_t));

    const bool are_buffers_allocated = (node != NULL);

    node_mutex = xSemaphoreCreateMutex();

    const bool are_semaphores_allocated = (node_mutex != NULL);

    humidity_timer          = xTimerCreate("humidity", pdMS_TO_TICKS(1000U), pdFALSE, NULL, board_T01_humidity_timer);
    reed_switch_timer       = xTimerCreate("reed_switch", pdMS_TO_TICKS(1000U), pdFALSE, NULL, board_T01_reed_switch_timer);
    lightning_block_timer   = xTimerCreate("lightning_block", pdMS_TO_TICKS(1000U), pdFALSE, NULL, board_T01_lightning_block_timer);
    light_timer             = xTimerCreate("light", pdMS_TO_TICKS(1000U), pdFALSE, NULL, board_T01_light_timer);
    display_timer           = xTimerCreate("display", pdMS_TO_TICKS(1000U), pdFALSE, NULL, board_T01_display_timer);
    warning_led_timer       = xTimerCreate("warning_led", pdMS_TO_TICKS(1000U), pdFALSE, NULL, board_T01_warning_led_timer);

    const bool are_timers_allocated = (humidity_timer != NULL) && (reed_switch_timer != NULL) &&
                                        (lightning_block_timer != NULL) && (light_timer != NULL) &&
                                        (display_timer != NULL) && (warning_led_timer != NULL);

    if ((are_buffers_allocated != true) || (are_semaphores_allocated != true) || (are_timers_allocated != true))
    {
        vPortFree((void*)node);
        vSemaphoreDelete(node_mutex);
        xTimerDelete(humidity_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(reed_switch_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(lightning_block_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(light_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(display_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(warning_led_timer, RTOS_TIMER_TICKS_TO_WAIT);

        std_error_catch_custom(error, STD_FAILURE, MALLOC_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    BaseType_t exit_code = xTaskCreate(board_T01_task, RTOS_TASK_NAME, RTOS_TASK_STACK_SIZE, NULL, RTOS_TASK_PRIORITY, &task);

    if (exit_code != pdPASS)
    {
        std_error_catch_custom(error, (int)exit_code, MALLOC_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}
