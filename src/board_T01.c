/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#include "board_T01.h"

#include <stdbool.h>
#include <limits.h>
#include <assert.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

#include "lfs.h"

#include "board.i2c_1.h"

#include "devices/mcp23017_expander.h"
#include "devices/w25q32bv_flash.h"
#include "devices/bme280_sensor.h"
#include "devices/ssd1306_display.h"

#include "node_T01.h"

#include "logger.h"
#include "std_error/std_error.h"


#define RTOS_TASK_STACK_SIZE    1024U       // 1024*4=4096 bytes
#define RTOS_TASK_PRIORITY      0U          // 0 - lowest, 4 - highest
#define RTOS_TASK_NAME          "board_T01" // 16 - max length

#define RTOS_TIMER_TICKS_TO_WAIT    (1U * 1000U)
#define REMOTE_BUTTON_QUEUE_SIZE    4U
#define PIR_HYSTERESIS_MS           (1U * 1000U) // 1 sec

#define DEFAULT_ERROR_TEXT  "Board T01 error"
#define MALLOC_ERROR_TEXT   "Board T01 memory allocation error"

#define REMOTE_BUTTON_NOTIFICATION      (1 << 0)
#define FRONT_PIR_NOTIFICATION          (1 << 1)
#define UPDATE_STATE_NOTIFICATION       (1 << 2)

#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


static TaskHandle_t task;
static SemaphoreHandle_t node_mutex;
static SemaphoreHandle_t remote_button_mutex;
static TimerHandle_t luminosity_timer;
static TimerHandle_t humidity_timer;
static TimerHandle_t reed_switch_timer;
static TimerHandle_t light_timer;
static TimerHandle_t display_timer;

static board_T01_config_t config;

static node_T01_t *node;
static board_remote_button_t latest_remote_button;
static board_led_color_t status_led_color;
static bool is_light_on;
static bool is_display_on;
static bool is_warning_led_on;


static int board_T01_malloc (std_error_t * const error);
static void board_T01_task (void *parameters);
static void board_T01_luminosity_timer (TimerHandle_t timer);
static void board_T01_humidity_timer (TimerHandle_t timer);
static void board_T01_reed_switch_timer (TimerHandle_t timer);
static void board_T01_light_timer (TimerHandle_t timer);
static void board_T01_display_timer (TimerHandle_t timer);

static void board_T01_front_pir_ISR ();

static void board_T01_enable_light_power ();
static void board_T01_disable_light_power ();

static void board_T01_enable_display_power ();
static void board_T01_disable_display_power ();
static void board_T01_draw_blue_display (node_T01_humidity_t const * const data);
static void board_T01_draw_yellow_display (node_T01_humidity_t const * const data);

static void board_T01_enable_warning_led_power ();
static void board_T01_disable_warning_led_power ();

int board_T01_init (board_T01_config_t const * const init_config, std_error_t * const error)
{
    assert(init_config                          != NULL);
    assert(init_config->mcp23017_expander       != NULL);
    assert(init_config->w25q32bv_flash          != NULL);
    assert(init_config->set_led_color_callback  != NULL);
    assert(init_config->get_luminosity_callback != NULL);
    assert(init_config->send_node_msg_callback  != NULL);

    config = *init_config;

    return board_T01_malloc(error);
}

void board_T01_task (void *parameters)
{
    UNUSED(parameters);
    
    std_error_t error;
    std_error_init(&error);

    status_led_color    = NO_COLOR;
    is_light_on         = false;
    is_display_on       = false;
    is_warning_led_on   = false;

    // Init node
    node_T01_init(node);

    // Init bme280 sensor
    {
        LOG("Board T01 : init bme280 sensor\r\n");

        bme280_sensor_config_t config;
        config.write_i2c_callback   = board_i2c_1_write_register;
        config.read_i2c_callback    = board_i2c_1_read_register;
        config.i2c_timeout_ms       = 1U * 1000U;
        config.delay_callback       = vTaskDelay;

        if (bme280_sensor_init(&config, &error) != STD_SUCCESS)
        {
            LOG("Board T01 : %s\r\n", error.text);
        }
    }

    while (true)
    {
        uint32_t notification;
        xTaskNotifyWait(0U, ULONG_MAX, &notification, 30U * 1000U);

        const uint32_t tick_count_ms = xTaskGetTickCount();

        if ((notification & REMOTE_BUTTON_NOTIFICATION) != 0U)
        {
            board_remote_button_t remote_button;

            xSemaphoreTake(remote_button_mutex, portMAX_DELAY);
            remote_button = latest_remote_button;
            xSemaphoreGive(remote_button_mutex);

            xSemaphoreTake(node_mutex, portMAX_DELAY);
            node_T01_process_remote_button(node, remote_button);
            xSemaphoreGive(node_mutex);
        }

        if ((notification & FRONT_PIR_NOTIFICATION) != 0U)
        {
            xSemaphoreTake(node_mutex, portMAX_DELAY);
            node_T01_process_front_pir(node, tick_count_ms);
            xSemaphoreGive(node_mutex);
        }

        // Update T01 states
        {
            node_T01_state_t node_state;

            // Try to send new messages
            while (true)
            {
                xSemaphoreTake(node_mutex, portMAX_DELAY);
                node_T01_get_state(node, &node_state, tick_count_ms);
                xSemaphoreGive(node_mutex);

                if (node_state.is_msg_to_send == true)
                {
                    node_msg_t send_msg;

                    xSemaphoreTake(node_mutex, portMAX_DELAY);
                    int exit_code = node_T01_get_msg(node, &send_msg, &error);
                    xSemaphoreGive(node_mutex);

                    if (exit_code != STD_SUCCESS)
                    {
                        LOG("Board T01 : %s\r\n", error.text);
                        break;
                    }

                    exit_code = config.send_node_msg_callback(&send_msg, &error);

                    if (exit_code != STD_SUCCESS)
                    {
                        LOG("Board T01 : %s\r\n", error.text);
                    }
                }
                else
                {
                    break;
                }
            }

            // Update light state
            if (node_state.is_light_on == true)
            {
                if (xTimerIsTimerActive(light_timer) == pdFALSE)
                {
                    xTimerChangePeriod(light_timer, 100U / portTICK_PERIOD_MS, RTOS_TIMER_TICKS_TO_WAIT);
                }
            }

            // Update display state
            if (node_state.is_display_on == true)
            {
                if (xTimerIsTimerActive(display_timer) == pdFALSE)
                {
                    xTimerChangePeriod(display_timer, 100U / portTICK_PERIOD_MS, RTOS_TIMER_TICKS_TO_WAIT);
                }
            }

            // Update warning LED state
            if (node_state.is_warning_led_on == true)
            {
                if (is_warning_led_on != true)
                {
                    is_warning_led_on = true;
                    board_T01_enable_warning_led_power();
                }
            }
            else
            {
                if (is_warning_led_on != false)
                {
                    is_warning_led_on = false;
                    board_T01_disable_warning_led_power();
                }
            }

            // Update status LED color
            if (node_state.status_led_color != status_led_color)
            {
                status_led_color = node_state.status_led_color;
                config.set_led_color_callback(status_led_color, NULL);
            }
        }

        LOG("Board T01 : loop\r\n");
    }

    return;
}

void board_T01_receive_node_msg (node_msg_t const * const rcv_msg)
{
    const uint32_t tick_count_ms = xTaskGetTickCount();

    xSemaphoreTake(node_mutex, portMAX_DELAY);
    node_T01_process_rcv_msg(node, rcv_msg, tick_count_ms);
    xSemaphoreGive(node_mutex);

    xTaskNotify(task, UPDATE_STATE_NOTIFICATION, eSetBits);

    return;
}

void board_T01_luminosity_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    if ((is_light_on != true) && (is_display_on != true))
    {
        std_error_t error;
        std_error_init(&error);

        board_T01_disable_warning_led_power();
        config.set_led_color_callback(NO_COLOR, NULL);

        vTaskDelay(100U);

        node_T01_luminosity_t luminosity;

        if (config.get_luminosity_callback(&luminosity.adc, &error) != STD_FAILURE)
        {
            luminosity.is_valid = true;

            LOG("Board T01 : luminosity adc = %lu\r\n", luminosity.adc);
        }
        else
        {
            luminosity.is_valid = false;

            LOG("Board T01 : %s\r\n", error.text);
        }

        uint32_t next_time_ms;

        xSemaphoreTake(node_mutex, portMAX_DELAY);
        node_T01_process_luminosity(node, &luminosity, &next_time_ms);
        xSemaphoreGive(node_mutex);

        is_light_on     = false;
        is_display_on   = false;

        xTimerChangePeriod(luminosity_timer, next_time_ms / portTICK_PERIOD_MS, RTOS_TIMER_TICKS_TO_WAIT);
    }
    else
    {
        xTimerStart(luminosity_timer, RTOS_TIMER_TICKS_TO_WAIT);
    }

    xTaskNotify(task, UPDATE_STATE_NOTIFICATION, eSetBits);

    return;
}

void board_T01_humidity_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    std_error_t error;
    std_error_init(&error);

    node_T01_humidity_t humidity;
    bme280_sensor_data_t data;

    if (bme280_sensor_read_data(&data, &error) != STD_FAILURE)
    {
        humidity.humidity_pct   = data.humidity_pct;
        humidity.pressure_hPa   = data.pressure_hPa;
        humidity.temperature_C  = data.temperature_C;
        humidity.is_valid       = true;

        LOG("Board T01 : humidity = %f %%\r\n", data.humidity_pct);
        LOG("Board T01 : temperature = %f C\r\n", data.temperature_C);
        LOG("Board T01 : pressure = %f hPa\r\n", data.pressure_hPa);
    }
    else
    {
        humidity.is_valid = false;

        LOG("Board T01 : %s\r\n", error.text);
    }

    uint32_t next_time_ms;

    xSemaphoreTake(node_mutex, portMAX_DELAY);
    node_T01_process_humidity(node, &humidity, &next_time_ms);
    xSemaphoreGive(node_mutex);

    xTimerChangePeriod(humidity_timer, next_time_ms / portTICK_PERIOD_MS, RTOS_TIMER_TICKS_TO_WAIT);

    xTaskNotify(task, UPDATE_STATE_NOTIFICATION, eSetBits);

    return;
}

void board_T01_reed_switch_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    // **************************************
    // Read state
    bool is_reed_switch_open = false;
    // **************************************

    uint32_t next_time_ms;

    xSemaphoreTake(node_mutex, portMAX_DELAY);
    node_T01_process_reed_switch(node, is_reed_switch_open, &next_time_ms);
    xSemaphoreGive(node_mutex);

    xTimerChangePeriod(reed_switch_timer, next_time_ms / portTICK_PERIOD_MS, RTOS_TIMER_TICKS_TO_WAIT);

    xTaskNotify(task, UPDATE_STATE_NOTIFICATION, eSetBits);

    return;
}

void board_T01_light_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    if (is_light_on != true)
    {
        is_light_on = true;
        board_T01_enable_light_power();

        uint32_t disable_time_ms;

        xSemaphoreTake(node_mutex, portMAX_DELAY);
        node_T01_get_light_data(node, &disable_time_ms);
        xSemaphoreGive(node_mutex);

        xTimerChangePeriod(light_timer, disable_time_ms / portTICK_PERIOD_MS, RTOS_TIMER_TICKS_TO_WAIT);
    }
    else
    {
        is_light_on = false;
        board_T01_disable_light_power();
    }

    return;
}

void board_T01_enable_light_power ()
{
    LOG("Board T01 : enable power for light\r\n");

    return;
}

void board_T01_disable_light_power ()
{
    LOG("Board T01 : disable power for light\r\n");

    return;
}

void board_T01_enable_warning_led_power ()
{
    LOG("Board T01 : enable power for warning LED\r\n");

    return;
}

void board_T01_disable_warning_led_power ()
{
    LOG("Board T01 : disable power for warning LED\r\n");

    return;
}

void board_T01_remote_control_ISR (board_remote_button_t remote_button)
{
    BaseType_t is_higher_priority_task_woken;

    xSemaphoreTakeFromISR(remote_button_mutex, &is_higher_priority_task_woken);
    latest_remote_button = remote_button;
    xSemaphoreGiveFromISR(remote_button_mutex, &is_higher_priority_task_woken);

    xTaskNotifyFromISR(task, REMOTE_BUTTON_NOTIFICATION, eSetBits, &is_higher_priority_task_woken);

    portYIELD_FROM_ISR(is_higher_priority_task_woken);

    return;
}

void board_T01_front_pir_ISR ()
{
    static TickType_t last_tick_count_ms = 0U;

    const TickType_t tick_count_ms = xTaskGetTickCountFromISR();

    if ((tick_count_ms - last_tick_count_ms) > PIR_HYSTERESIS_MS)
    {
        BaseType_t is_higher_priority_task_woken;

        xTaskNotifyFromISR(task, FRONT_PIR_NOTIFICATION, eSetBits, &is_higher_priority_task_woken);

        portYIELD_FROM_ISR(is_higher_priority_task_woken);
    }

    return;
}

void board_T01_display_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    if (is_display_on != true)
    {
        is_display_on = true;
        board_T01_enable_display_power();
        
        node_T01_humidity_t data;
        uint32_t disable_time_ms;

        xSemaphoreTake(node_mutex, portMAX_DELAY);
        node_T01_get_display_data(node, &data, &disable_time_ms);
        xSemaphoreGive(node_mutex);

        vTaskDelay(1U * 1000U);

        board_T01_draw_yellow_display(&data);
        board_T01_draw_blue_display(&data);

        xTimerChangePeriod(display_timer, disable_time_ms / portTICK_PERIOD_MS, RTOS_TIMER_TICKS_TO_WAIT);
    }
    else
    {
        is_display_on = false;
        board_T01_disable_display_power();
    }

    return;
}

void board_T01_enable_display_power ()
{
    LOG("Board T01 : enable power for display\r\n");

    return;
}

void board_T01_disable_display_power ()
{
    LOG("Board T01 : disable power for display\r\n");

    return;
}

void board_T01_draw_blue_display (node_T01_humidity_t const * const data)
{
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
    std_error_t error;
    std_error_init(&error);

    uint8_t ssd1306_pixel_buffer[SSD1306_DISPLAY_PIXEL_BUFFER_SIZE];

    ssd1306_display_config_t config;
    config.write_i2c_callback       = board_i2c_1_write;
    config.write_i2c_dma_callback   = NULL;
    config.i2c_timeout_ms           = 1U * 1000U;
    config.pixel_buffer             = ssd1306_pixel_buffer;
    config.device_address           = SSD1306_DISPLAY_ADDRESS_2;

    ssd1306_display_t ssd1306_display;
    int exit_code = ssd1306_display_init(&ssd1306_display, &config, &error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Board T01 : init blue display = %s\r\n", error.text);

        return;
    }

    ssd1306_display_reset_buffer(&ssd1306_display);

    exit_code = ssd1306_display_update_full_screen(&ssd1306_display, &error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Board T01 : clear blue display = %s\r\n", error.text);
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

    exit_code = ssd1306_display_update_full_screen(&ssd1306_display, &error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Board T01 : draw blue display = %s\r\n", error.text);
    }

    return;
}

void board_T01_draw_yellow_display (node_T01_humidity_t const * const data)
{
    //const uint8_t hum_text[] = {  0xE2, 0xEB, 0xE0, 0xE6, 0xED, 0xEE, 0xF1, 0xF2, 0xFC };
    //const uint8_t press_text[] = {  0xE4, 0xE0, 0xE2, 0xEB, 0xE5, 0xED, 0xE8, 0xE5 };
    //const uint8_t press_text[] = {  0xE3, 0xCF, 0xC0 };

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

    std_error_t error;
    std_error_init(&error);

    uint8_t ssd1306_pixel_buffer[SSD1306_DISPLAY_PIXEL_BUFFER_SIZE];

    ssd1306_display_config_t config;
    config.write_i2c_callback       = board_i2c_1_write;
    config.write_i2c_dma_callback   = NULL;
    config.i2c_timeout_ms           = 1U * 1000U;
    config.pixel_buffer             = ssd1306_pixel_buffer;
    config.device_address           = SSD1306_DISPLAY_ADDRESS_1;

    ssd1306_display_t ssd1306_display;
    int exit_code = ssd1306_display_init(&ssd1306_display, &config, &error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Board T01 : init yellow display = %s\r\n", error.text);

        return;
    }

    ssd1306_display_reset_buffer(&ssd1306_display);

    exit_code = ssd1306_display_update_full_screen(&ssd1306_display, &error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Board T01 : clear yellow display = %s\r\n", error.text);
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

    exit_code = ssd1306_display_update_full_screen(&ssd1306_display, &error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Board T01 : draw yellow display = %s\r\n", error.text);
    }

    return;
}


int board_T01_malloc (std_error_t * const error)
{
    node = (node_T01_t*)pvPortMalloc(sizeof(node_T01_t));

    const bool are_buffers_allocated = (node != NULL);

    node_mutex          = xSemaphoreCreateMutex();
    remote_button_mutex = xSemaphoreCreateMutex();

    const bool are_semaphores_allocated = (node_mutex != NULL) && (remote_button_mutex != NULL);

    luminosity_timer    = xTimerCreate("luminosity", 0U, pdFALSE, NULL, board_T01_luminosity_timer);
    humidity_timer      = xTimerCreate("humidity", 0U, pdFALSE, NULL, board_T01_humidity_timer);
    reed_switch_timer   = xTimerCreate("reed_switch", 0U, pdFALSE, NULL, board_T01_reed_switch_timer);
    display_timer       = xTimerCreate("display", 0U, pdFALSE, NULL, board_T01_display_timer);
    light_timer         = xTimerCreate("light", 0U, pdFALSE, NULL, board_T01_light_timer);

    const bool are_timers_allocated = (luminosity_timer != NULL) && (humidity_timer != NULL) &&
                                        (reed_switch_timer != NULL) && (display_timer != NULL);

    if ((are_buffers_allocated != true) || (are_semaphores_allocated != true) || (are_timers_allocated != true))
    {
        vPortFree((void*)node);
        vSemaphoreDelete(node_mutex);
        vSemaphoreDelete(remote_button_mutex);
        xTimerDelete(luminosity_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(humidity_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(reed_switch_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(display_timer, RTOS_TIMER_TICKS_TO_WAIT);
        xTimerDelete(light_timer, RTOS_TIMER_TICKS_TO_WAIT);

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



/* NOTE: LFS using example

    lfs_t lfs;
    lfs_file_t file;
    printf("Start\r\n");
    int err = lfs_mount(&lfs, &cfg);
    printf("Mount : %d\r\n", err);
    
    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err)
    {
        printf("Format\r\n");
        err = lfs_format(&lfs, &cfg);
        printf("Mount: %d\r\n", err);
        lfs_mount(&lfs, &cfg);
    }

    //lfs_file_open(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    lfs_mkdir(&lfs, "config");
    struct lfs_file_config config = { 0 };
    config.buffer = (void*)lfs_file_buffer;
    lfs_file_opencfg(&lfs, &file, "config/boot_count", LFS_O_RDWR | LFS_O_CREAT, &config);

    uint32_t boot_count = 0;
    lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

    // update boot count
    boot_count += 1;
    lfs_file_rewind(&lfs, &file);
    lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));

    // remember the storage is not updated until the file is closed successfully
    lfs_file_close(&lfs, &file);

    // release any resources we were using
    printf("Umount\r\n");
    lfs_unmount(&lfs);

    printf("boot_count: %d\r\n", boot_count);
*/
