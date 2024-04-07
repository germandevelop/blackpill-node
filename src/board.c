/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "board.h"

#include <limits.h>
#include <assert.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

#include "board.type.h"
#include "board.uart_2.h"
#include "board.spi_1.h"
#include "board.i2c_1.h"
#include "board.adc_1.h"
#include "board.gpio_a.h"
#include "board.gpio_c.h"
#include "board.exti_1.h"
#include "board.exti_2.h"
#include "board.timer_2.h"
#include "board.timer_3.h"
#include "board_factory.h"

#include "devices/mcp23017_expander.h"
#include "devices/vs1838_control.h"

#include "storage.h"
#include "node.h"
#include "node/node.list.h"
#include "tcp_client.h"

#include "version.h"

#include "logger.h"
#include "std_error/std_error.h"


#define RTOS_TASK_STACK_SIZE    512U    // 512*4=2048 bytes
#define RTOS_TASK_PRIORITY      4U      // 0 - lowest, 4 - highest
#define RTOS_TASK_NAME          "board" // 16 - max length

#define RTOS_TIMER_TICKS_TO_WAIT (100U)

#define STATUS_LED_NOTIFICATION     (1 << 0)
#define REMOTE_BUTTON_NOTIFICATION  (1 << 1)

#define UART_TIMEOUT_MS (1U * 1000U)    // 1 sec
#define SPI_TIMEOUT_MS  (1U * 1000U)    // 1 sec
#define I2C_TIMEOUT_MS  (1U * 1000U)    // 1 sec

#define PHOTORESISTOR_MEAUSEREMENT_COUNT    5U
#define PHOTORESISTOR_INITIAL_PERIOD_MS     (1U * 60U * 1000U) // 1 min

#define DEFAULT_ERROR_TEXT  "Board error"
#define MALLOC_ERROR_TEXT   "Board memory allocation error"

#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


static TaskHandle_t task;
static SemaphoreHandle_t status_led_mutex;
static SemaphoreHandle_t remote_button_mutex;
static TimerHandle_t photoresistor_timer;
static SemaphoreHandle_t spi_1_mutex;
static SemaphoreHandle_t i2c_1_mutex;

static board_config_t config;
static board_setup_t setup;

static board_led_color_t status_led_color;
static mcp23017_expander_t mcp23017_expander;
static storage_t storage;
static vs1838_control_t vs1838_control;
static board_remote_button_t latest_remote_button;


static int board_malloc (std_error_t * const error);
static void board_task (void *parameters);
static void board_photoresistor_timer (TimerHandle_t timer);

static void board_init_logger ();
static void board_init_status_led ();
static void board_init_expander ();
static void board_init_storage ();
static void board_init_node ();
static void board_init_extension ();
static void board_init_tcp_client ();
static void board_init_remote_control ();

static void board_update_status_led (board_led_color_t led_color);
static int board_set_status_led_color (board_led_color_t led_color, std_error_t * const error);
static void board_remote_control_ISR (uint32_t captured_value);

static void board_uart_2_print (const uint8_t *data, uint16_t data_size);
static void board_i2c_1_lock ();
static void board_i2c_1_unlock ();
static void board_spi_1_lock ();
static void board_spi_1_unlock ();

int board_init (board_config_t const * const init_config, std_error_t * const error)
{
    assert(init_config                              != NULL);
    assert(init_config->refresh_watchdog_callback   != NULL);
    assert(init_config->watchdog_timeout_ms         != 0U);

    config = *init_config;

#ifndef NDEBUG
    board_init_logger();
#endif // NDEBUG

    return board_malloc(error);
}

void board_task (void *parameters)
{
    UNUSED(parameters);

    LOG("Board : firmware version = %s.%s.%s\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

    board_factory_build_setup(&setup);

    board_init_status_led();
    board_init_expander();
    board_init_storage();
    board_init_node();
    board_init_extension();
    board_init_tcp_client();
    board_init_remote_control();

    std_error_t error;
    std_error_init(&error);

    xTimerStart(photoresistor_timer, RTOS_TIMER_TICKS_TO_WAIT);

    while (true)
    {
        uint32_t notification;
        xTaskNotifyWait(0U, ULONG_MAX, &notification, config.watchdog_timeout_ms);

        if ((notification & STATUS_LED_NOTIFICATION) != 0U)
        {
            xSemaphoreTake(status_led_mutex, portMAX_DELAY);
            const board_led_color_t current_led_color = status_led_color;
            xSemaphoreGive(status_led_mutex);

            if (board_set_status_led_color(current_led_color, &error) != STD_SUCCESS)
            {
                LOG("Board [status_led] : %s\r\n", error.text);
            }
        }

        if ((notification & REMOTE_BUTTON_NOTIFICATION) != 0U)
        {
            xSemaphoreTake(remote_button_mutex, portMAX_DELAY);
            const board_remote_button_t current_remote_button = latest_remote_button;
            xSemaphoreGive(remote_button_mutex);

            LOG("Board [remote_control] : button = %d\r\n", current_remote_button);

            setup.process_remote_button_callback(current_remote_button);
        }

        LOG("Board [watchdog] : feed\r\n");

        config.refresh_watchdog_callback();
    }
    
    return;
}


void board_init_logger ()
{
    int exit_code = board_uart_2_init(NULL);

    logger_config_t logger_config;
    logger_config.write_array_callback = board_uart_2_print;

    if (exit_code != STD_SUCCESS)
    {
        logger_config.write_array_callback = NULL;
    }

    logger_init(&logger_config);

    return;
}

void board_init_status_led ()
{
    std_error_t error;
    std_error_init(&error);

    LOG("Board [TIMER_2] : init\r\n");

    board_timer_2_config_t timer_2_config;
    timer_2_config.ic_isr_callback = board_remote_control_ISR;

    if (board_timer_2_init(&timer_2_config, &error) != STD_SUCCESS)
    {
        LOG("Board [TIMER_2] : %s\r\n", error.text);
    }

    LOG("Board [TIMER_3] : init\r\n");

    if (board_timer_3_init(&error) != STD_SUCCESS)
    {
        LOG("Board [TIMER_3] : %s\r\n", error.text);
    }

    LOG("Board [status_led] : init\r\n");

    status_led_color = BLUE_COLOR;

    if (board_set_status_led_color(status_led_color, &error) != STD_SUCCESS)
    {
        LOG("Board [status_led] : %s\r\n", error.text);
    }

    return;
}

void board_init_expander ()
{
    std_error_t error;
    std_error_init(&error);

    LOG("Board [I2C_1] : init\r\n");

    if (board_i2c_1_init(&error) != STD_SUCCESS)
    {
        LOG("Board [I2C_1] : %s\r\n", error.text);
    }

    LOG("Board [expander] : init (MCP23017)\r\n");

    mcp23017_expander_config_t config;
    config.write_i2c_callback   = board_i2c_1_write_register;
    config.read_i2c_callback    = board_i2c_1_read_register;
    config.i2c_timeout_ms       = I2C_TIMEOUT_MS;

    if (mcp23017_expander_init(&mcp23017_expander, &config, &error) != STD_SUCCESS)
    {
        LOG("Board [expander] : %s\r\n", error.text);
    }

    LOG("Board [expander] : setup port A\r\n");

    if (mcp23017_expander_set_port_direction(&mcp23017_expander, PORT_A, OUTPUT_DIRECTION, &error) != STD_SUCCESS)
    {
        LOG("Board [expander] : %s\r\n", error.text);
    }
    if (mcp23017_expander_set_port_out(&mcp23017_expander, PORT_A, LOW_GPIO, &error) != STD_SUCCESS)
    {
        LOG("Board [expander] : %s\r\n", error.text);
    }

    LOG("Board [expander] : setup port B\r\n");

    if (mcp23017_expander_set_port_direction(&mcp23017_expander, PORT_B, OUTPUT_DIRECTION, &error) != STD_SUCCESS)
    {
        LOG("Board [expander] : %s\r\n", error.text);
    }
    if (mcp23017_expander_set_port_out(&mcp23017_expander, PORT_B, LOW_GPIO, &error) != STD_SUCCESS)
    {
        LOG("Board [expander] : %s\r\n", error.text);
    }

    return;
}

void board_init_storage ()
{
    std_error_t error;
    std_error_init(&error);

    LOG("Board [GPIO_A] : init\r\n");

    board_gpio_a_init();

    LOG("Board [SPI_1] : init\r\n");

    if (board_spi_1_init(&error) != STD_SUCCESS)
    {
        LOG("Board [SPI_1] : %s\r\n", error.text);
    }

    LOG("Board [storage] : init\r\n");

    storage_config_t config;
    config.spi_lock_callback        = board_spi_1_lock;
    config.spi_unlock_callback      = board_spi_1_unlock;
    config.spi_select_callback      = board_gpio_a_pin_4_reset;
    config.spi_unselect_callback    = board_gpio_a_pin_4_set;
    config.spi_tx_rx_callback       = board_spi_1_read_write;
    config.spi_timeout_ms           = SPI_TIMEOUT_MS;
    config.delay_callback           = vTaskDelay;

    if (storage_init(&storage, &config, &error) != STD_SUCCESS)
    {
        LOG("Board [storage] : %s\r\n", error.text);
    }

    return;
}

void board_init_extension ()
{
    std_error_t error;
    std_error_init(&error);

    LOG("Board [extension] : init\r\n");

    board_extension_config_t config;
    config.mcp23017_expander            = &mcp23017_expander;
    config.storage                      = &storage;
    config.lock_i2c_1_callback          = board_i2c_1_lock;
    config.unlock_i2c_1_callback        = board_i2c_1_unlock;
    config.update_status_led_callback   = board_update_status_led;
    config.send_node_msg_callback       = node_send_msg;

    if (setup.init_extension_callback(&config, &error) != STD_SUCCESS)
    {
        LOG("Board [extension] : %s\r\n", error.text);
    }

    return;
}

void board_init_node ()
{
    std_error_t error;
    std_error_init(&error);

    LOG("Board [node] : init\r\n");

    node_config_t config;
    config.id                       = setup.node_id;
    config.receive_msg_callback     = setup.process_msg_callback;
    config.send_tcp_msg_callback    = tcp_client_send_message;

    if (node_init(&config, &error) != STD_SUCCESS)
    {
        LOG("Board [node] : %s\r\n", error.text);
    }

    return;
}

void board_init_tcp_client ()
{
    std_error_t error;
    std_error_init(&error);

    LOG("Board [GPIO_C] : init\r\n");

    board_gpio_c_init();

    LOG("Board [EXTI_1] : init\r\n");

    if (board_exti_1_init(tcp_client_ISR, &error) != STD_SUCCESS)
    {
        LOG("Board [EXTI_1] : %s\r\n", error.text);
    }

    LOG("Board [tcp_client] : init\r\n");

    tcp_client_config_t config = { 0 };

    config.process_msg_callback = node_receive_tcp_msg;

    config.spi_lock_callback        = board_spi_1_lock;
    config.spi_unlock_callback      = board_spi_1_unlock;
    config.spi_select_callback      = board_gpio_c_pin_13_reset;
    config.spi_unselect_callback    = board_gpio_c_pin_13_set;
    config.spi_read_callback        = board_spi_1_read;
    config.spi_write_callback       = board_spi_1_write;
    config.spi_timeout_ms           = SPI_TIMEOUT_MS;

    config.mac_address[0] = 0xEA;
    config.mac_address[1] = 0x11;
    config.mac_address[2] = 0x22;
    config.mac_address[3] = 0x33;
    config.mac_address[4] = 0x44;
    config.mac_address[5] = 0xEA;

    config.ip_address[0] = 192;
    config.ip_address[1] = 168;
    config.ip_address[2] = 1;
    config.ip_address[3] = 123;

    config.netmask[0] = 255;
    config.netmask[1] = 255;
    config.netmask[2] = 255;
    config.netmask[3] = 0;

    config.server_ip[0] = 192;
    config.server_ip[1] = 168;
    config.server_ip[2] = 1;
    config.server_ip[3] = 105;

    config.server_port = host_port;

    if (tcp_client_init(&config, &error) != STD_SUCCESS)
    {
        LOG("Board [tcp_client] : %s\r\n", error.text);
    }

    return;
}

void board_init_remote_control ()
{
    std_error_t error;
    std_error_init(&error);

    LOG("Board [remote_control] : init (VS1838)\r\n");

    vs1838_control_config_t config;
    config.start_bit    = 1155U;
    config.one_bit      = 190U;
    config.zero_bit     = 99U;
    config.threshold    = 30U;

    vs1838_control_init(&vs1838_control, &config);

    bool is_remote_control_enabled;
    setup.is_remote_control_enabled_callback(&is_remote_control_enabled);

    if (is_remote_control_enabled == true)
    {
        if (board_timer_2_start_channel_3(&error) != STD_SUCCESS)
        {
            LOG("Board [remote_control] : %s\r\n", error.text);
        }
    }

    return;
}


void board_update_status_led (board_led_color_t led_color)
{
    if (led_color != status_led_color)
    {
        xSemaphoreTake(status_led_mutex, portMAX_DELAY);
        const board_led_color_t current_led_color = status_led_color;
        xSemaphoreGive(status_led_mutex);

        if (led_color != current_led_color)
        {
            LOG("Board [status_led] : update\r\n");

            xSemaphoreTake(status_led_mutex, portMAX_DELAY);
            status_led_color = led_color;
            xSemaphoreGive(status_led_mutex);

            xTaskNotify(task, STATUS_LED_NOTIFICATION, eSetBits);
        }
    }

    return;
}

int board_set_status_led_color (board_led_color_t led_color, std_error_t * const error)
{
    board_timer_2_stop_channel_2(); // Green
    board_timer_3_deinit();         // Red + Blue

    int exit_code = STD_SUCCESS;
    
    if (led_color == GREEN_COLOR)
    {
        exit_code = board_timer_2_start_channel_2(error);
    }
    else if ((led_color == BLUE_COLOR) || (led_color == RED_COLOR))
    {
        exit_code = board_timer_3_init(error);

        if (exit_code == STD_SUCCESS)
        {
            if (led_color == BLUE_COLOR)
            {
                exit_code = board_timer_3_start_channel_2(error);
            }
            else if (led_color == RED_COLOR)
            {
                exit_code = board_timer_3_start_channel_1(error);
            }
        }
    }
    return exit_code;
}

void board_photoresistor_timer (TimerHandle_t timer)
{
    UNUSED(timer);

    const uint32_t initial_period_ms    = 1U * 1000U;
    const uint32_t iteration_period_ms  = 500U;
    const uint32_t adc_timeout_ms       = 200U;

    const uint32_t disable_period_ms = initial_period_ms + ((iteration_period_ms + adc_timeout_ms) * PHOTORESISTOR_MEAUSEREMENT_COUNT);

    bool is_lightning_disabled;
    setup.disable_lightning_callback(disable_period_ms, &is_lightning_disabled);

    if (is_lightning_disabled == true)
    {
        uint32_t divider_adc;
        bool is_divider_adc_valid = false;

        // Collect adc data
        {
            std_error_t error;
            std_error_init(&error);

            xSemaphoreTake(status_led_mutex, portMAX_DELAY);

            if (board_set_status_led_color(NO_COLOR, &error) == STD_SUCCESS)
            {
                if (board_adc_1_init(&error) == STD_SUCCESS)
                {
                    vTaskDelay(pdMS_TO_TICKS(initial_period_ms));

                    uint32_t adc_buffer         = 0U;
                    uint32_t adc_buffer_size    = 0U;

                    for (size_t i = 0U; i < PHOTORESISTOR_MEAUSEREMENT_COUNT; ++i)
                    {
                        vTaskDelay(pdMS_TO_TICKS(iteration_period_ms));

                        uint32_t adc_value;

                        if (board_adc_1_read_value(&adc_value, adc_timeout_ms, &error) == STD_SUCCESS)
                        {
                            LOG("Board [photoresistor] : adc = %lu\r\n", adc_value);

                            adc_buffer += adc_value;
                            ++adc_buffer_size;
                        }
                        else
                        {
                            LOG("Board [photoresistor] : %s\r\n", error.text);
                        }
                    }

                    if (adc_buffer_size != 0U)
                    {
                        divider_adc = adc_buffer / adc_buffer_size;
                        is_divider_adc_valid = true;
                    }
                }
                else
                {
                    LOG("Board [photoresistor] : %s\r\n", error.text);
                }

                board_adc_1_deinit();
            }
            else
            {
                LOG("Board [status_led] : %s\r\n", error.text);
            }

            if (board_set_status_led_color(status_led_color, &error) != STD_SUCCESS)
            {
                LOG("Board [status_led] : %s\r\n", error.text);
            }

            xSemaphoreGive(status_led_mutex);
        }

        // Calculate resistance and voltage
        if (is_divider_adc_valid == true)
        {
            const uint32_t adc_max_value        = 4095U;
            const float supply_voltage_V        = 3.3F;
            const float divider_resistance_Ohm  = 10000.0F;

            const uint32_t adc_value = adc_max_value - divider_adc;

            photoresistor_data_t data;
            data.voltage_V  = supply_voltage_V * ((float)(adc_value) / (float)(adc_max_value));

            const float divider_voltage_V = supply_voltage_V - data.voltage_V;
            const float current_A = divider_voltage_V / divider_resistance_Ohm;

            data.resistance_Ohm = (uint32_t)(data.voltage_V / current_A);

            LOG("Board [photoresistor] : voltage = %.2f V\r\n", data.voltage_V);
            LOG("Board [photoresistor] : resistance = %lu Ohm\r\n", data.resistance_Ohm);

            uint32_t timer_period_ms;
            setup.process_photoresistor_data_callback(&data, &timer_period_ms);

            xTimerChangePeriod(photoresistor_timer, pdMS_TO_TICKS(timer_period_ms), RTOS_TIMER_TICKS_TO_WAIT);
        }
        else
        {
            xTimerStart(photoresistor_timer, RTOS_TIMER_TICKS_TO_WAIT);
        }
    }

    return;
}

void board_remote_control_ISR (uint32_t captured_value)
{
    static const uint32_t button_table[UNKNOWN_BUTTON] = 
    {
        [ZERO_BUTTON]   = ZERO_BUTTON_CODE,
        [ONE_BUTTON]    = ONE_BUTTON_CODE,
        [TWO_BUTTON]    = TWO_BUTTON_CODE,
        [THREE_BUTTON]  = THREE_BUTTON_CODE,
        [FOUR_BUTTON]   = FOUR_BUTTON_CODE,
        [FIVE_BUTTON]   = FIVE_BUTTON_CODE,
        [SIX_BUTTON]    = SIX_BUTTON_CODE,
        [SEVEN_BUTTON]  = SEVEN_BUTTON_CODE,
        [EIGHT_BUTTON]  = EIGHT_BUTTON_CODE,
        [NINE_BUTTON]   = NINE_BUTTON_CODE,
        [STAR_BUTTON]   = STAR_BUTTON_CODE,
        [GRID_BUTTON]   = GRID_BUTTON_CODE,
        [UP_BUTTON]     = UP_BUTTON_CODE,
        [LEFT_BUTTON]   = LEFT_BUTTON_CODE,
        [OK_BUTTON]     = OK_BUTTON_CODE,
        [RIGHT_BUTTON]  = RIGHT_BUTTON_CODE,
        [DOWN_BUTTON]   = DOWN_BUTTON_CODE
    };

    vs1838_control_process_bit(&vs1838_control, captured_value);

    bool is_frame_ready;
    vs1838_control_is_frame_ready(&vs1838_control, &is_frame_ready);

    if (is_frame_ready == true)
    {
        uint32_t button_code;
        vs1838_control_get_frame(&vs1838_control, &button_code);
        vs1838_control_reset_frame(&vs1838_control);

        board_remote_button_t remote_button = UNKNOWN_BUTTON;

        for (size_t i = 0U; i < ARRAY_SIZE(button_table); ++i)
        {
            if (button_table[i] == button_code)
            {
                remote_button = (board_remote_button_t)(i);

                break;
            }
        }

        BaseType_t is_higher_priority_task_woken;

        xSemaphoreTakeFromISR(remote_button_mutex, &is_higher_priority_task_woken);
        latest_remote_button = remote_button;
        xSemaphoreGiveFromISR(remote_button_mutex, &is_higher_priority_task_woken);

        xTaskNotifyFromISR(task, REMOTE_BUTTON_NOTIFICATION, eSetBits, &is_higher_priority_task_woken);

        portYIELD_FROM_ISR(is_higher_priority_task_woken);
    }

    return;
}


int board_malloc (std_error_t * const error)
{
    status_led_mutex    = xSemaphoreCreateMutex();
    remote_button_mutex = xSemaphoreCreateMutex();
    spi_1_mutex         = xSemaphoreCreateMutex();
    i2c_1_mutex         = xSemaphoreCreateMutex();

    const bool are_semaphores_allocated = (status_led_mutex != NULL) && (remote_button_mutex != NULL) &&
                                            (spi_1_mutex != NULL) && (i2c_1_mutex != NULL);

    photoresistor_timer = xTimerCreate("photoresistor", pdMS_TO_TICKS(PHOTORESISTOR_INITIAL_PERIOD_MS), pdFALSE, NULL, board_photoresistor_timer);

    const bool are_timers_allocated = (photoresistor_timer != NULL);

    if ((are_semaphores_allocated != true) || (are_timers_allocated != true))
    {
        vSemaphoreDelete(status_led_mutex);
        vSemaphoreDelete(remote_button_mutex);
        vSemaphoreDelete(spi_1_mutex);
        vSemaphoreDelete(i2c_1_mutex);
        xTimerDelete(photoresistor_timer, RTOS_TIMER_TICKS_TO_WAIT);

        std_error_catch_custom(error, STD_FAILURE, MALLOC_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    BaseType_t exit_code = xTaskCreate(board_task, RTOS_TASK_NAME, RTOS_TASK_STACK_SIZE, NULL, RTOS_TASK_PRIORITY, &task);

    if (exit_code != pdPASS)
    {
        std_error_catch_custom(error, (int)exit_code, MALLOC_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}



void board_uart_2_print (const uint8_t *data, uint16_t data_size)
{
    board_uart_2_write(data, data_size, UART_TIMEOUT_MS, NULL);

    return;
}

void board_i2c_1_lock ()
{
    xSemaphoreTake(i2c_1_mutex, portMAX_DELAY);

    return;
}

void board_i2c_1_unlock ()
{
    xSemaphoreGive(i2c_1_mutex);

    return;
}

void board_spi_1_lock ()
{
    xSemaphoreTake(spi_1_mutex, portMAX_DELAY);

    return;
}

void board_spi_1_unlock ()
{
    xSemaphoreGive(spi_1_mutex);

    return;
}
