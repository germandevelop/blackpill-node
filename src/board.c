/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "board.h"

#include <limits.h>
#include <assert.h>

#include "stm32f4xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

#include "lfs.h"

#include "board.types.h"
#include "board.uart_2.h"
#include "board.spi_1.h"
#include "board.i2c_1.h"
#include "board.adc_1.h"
#include "board.exti_1.h"
#include "board.exti_2.h"
#include "board.timer_2.h"
#include "board.timer_3.h"

#include "devices/mcp23017_expander.h"
#include "devices/w25q32bv_flash.h"
#include "devices/vs1838_control.h"

#include "board_T01.h"
#include "node.h"
#include "tcp_client.h"

#include "logger.h"
#include "std_error/std_error.h"


#define RTOS_TASK_STACK_SIZE    512U    // 512*4=2048 bytes
#define RTOS_TASK_PRIORITY      4U      // 0 - lowest, 4 - highest
#define RTOS_TASK_NAME          "board" // 16 - max length

#define RTOS_TIMER_TICKS_TO_WAIT  (1U * 1000U)

#define STATUS_LED_NOTIFICATION     (1 << 0)
#define REMOTE_BUTTON_NOTIFICATION  (1 << 1)

#define DEFAULT_ERROR_TEXT  "Board error"
#define MALLOC_ERROR_TEXT   "Board memory allocation error"

#define LFS_MIN_READ_BLOCK_SIZE 16U
#define LFS_MIN_PROG_BLOCK_SIZE 16U
#define LFS_ERASE_CYCLES        500
#define LFS_CACHE_SIZE          512U
#define LFS_LOOKAHEAD_SIZE      128U

#define I2C_TIMEOUT_MS                          (1U * 1000U)    // 1 sec
#define PHOTORESISTOR_MEAUSEREMENT_COUNT        5U
#define PHOTORESISTOR_MEAUSEREMENT_TIMEOUT_MS   (2U * 1000U)
#define PHOTORESISTOR_INITIAL_PERIOD_MS         (30U * 1000U)   // 30 sec

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


static TaskHandle_t task;
static SemaphoreHandle_t status_led_mutex;
static SemaphoreHandle_t remote_button_mutex;
static TimerHandle_t photoresistor_timer;

static board_config_t config;

static mcp23017_expander_t mcp23017_expander;
static w25q32bv_flash_t w25q32bv_flash;
static vs1838_control_t vs1838_control;
static board_led_color_t status_led_color;
static board_remote_button_t latest_remote_button;

static uint8_t lfs_read_buffer[LFS_CACHE_SIZE];
static uint8_t lfs_prog_buffer[LFS_CACHE_SIZE];
static uint8_t lfs_lookahead_buffer[LFS_LOOKAHEAD_SIZE];
//static uint8_t lfs_file_buffer[LFS_CACHE_SIZE];


static int board_malloc (std_error_t * const error);
static void board_task (void *parameters);
static void board_photoresistor_timer (TimerHandle_t timer);

static void board_init_logger ();
static void board_init_filesystem ();
static void board_init_tcp_client ();
static void board_init_expander ();
static void board_init_remote_control ();

static void board_print_uart_2 (const uint8_t *data, uint16_t data_size);

static void board_spi_1_init_select_gpio ();
static void board_w25q32bv_spi_1_select ();
static void board_w25q32bv_spi_1_unselect ();
static void board_w5500_spi_1_select ();
static void board_w5500_spi_1_unselect ();
static void board_spi_1_lock ();
static void board_spi_1_unlock ();

static void board_i2c_1_lock ();
static void board_i2c_1_unlock ();

static void board_timer_2_ic_isr_callback (uint32_t captured_value);

static void board_update_status_led (board_led_color_t led_color);
static int board_set_status_led_color (board_led_color_t led_color, std_error_t * const error);
static int board_read_photoresistor (uint32_t * const photoresistor_adc, std_error_t * const error);

static int board_lfs_block_device_read (const struct lfs_config *config, lfs_block_t sector_number, lfs_off_t sector_offset, void *raw_data, lfs_size_t size);
static int board_lfs_block_device_prog (const struct lfs_config *config, lfs_block_t sector_number, lfs_off_t sector_offset, const void *raw_data, lfs_size_t size);
static int board_lfs_block_device_erase (const struct lfs_config *config, lfs_block_t sector_number);
static int board_lfs_block_device_sync (const struct lfs_config *config);
static int board_lfs_block_device_lock (const struct lfs_config *config);
static int board_lfs_block_device_unlock (const struct lfs_config *config);
static void board_lfs_init_config (struct lfs_config *lfs_config);

int board_init (board_config_t const * const init_config, std_error_t * const error)
{
#ifndef NDEBUG
    board_init_logger();
#endif // NDEBUG

    assert(init_config != NULL);
    assert(init_config->refresh_watchdog_callback   != NULL);
    assert(init_config->watchdog_timeout_ms         != 0U);

    config = *init_config;

    return board_malloc(error);
}

void board_task (void *parameters)
{
    UNUSED(parameters);

    std_error_t error;
    std_error_init(&error);

    status_led_color = NO_COLOR;

    board_init_expander();
    board_init_filesystem();
    board_init_tcp_client();
    board_init_remote_control();

    {
        node_config_t config;
        config.id                       = NODE_T01;
        config.receive_msg_callback     = board_T01_process_rcv_node_msg;
        config.send_tcp_msg_callback    = tcp_client_send_message;

        if (node_init(&config, &error) != STD_SUCCESS)
        {
            LOG("Board : %s\r\n", error.text);
        }
    }

    {
        board_T01_config_t config;
        config.mcp23017_expander            = &mcp23017_expander;
        config.w25q32bv_flash               = &w25q32bv_flash;
        config.update_status_led_callback   = board_update_status_led;
        config.send_node_msg_callback       = node_send_msg;

        if (board_T01_init(&config, &error) != STD_SUCCESS)
        {
            LOG("Board : %s\r\n", error.text);
        }
    }

    xTimerStart(photoresistor_timer, RTOS_TIMER_TICKS_TO_WAIT);

    while (true)
    {
        uint32_t notification;
        xTaskNotifyWait(0U, ULONG_MAX, &notification, config.watchdog_timeout_ms);

        if ((notification & STATUS_LED_NOTIFICATION) != 0U)
        {
            board_led_color_t current_led_color;

            xSemaphoreTake(status_led_mutex, portMAX_DELAY);
            current_led_color = status_led_color;
            xSemaphoreGive(status_led_mutex);

            if (board_set_status_led_color(current_led_color, &error) != STD_SUCCESS)
            {
                LOG("Board : %s\r\n", error.text);
            }
        }

        if ((notification & REMOTE_BUTTON_NOTIFICATION) != 0U)
        {
            board_remote_button_t current_remote_button;

            xSemaphoreTake(remote_button_mutex, portMAX_DELAY);
            current_remote_button = latest_remote_button;
            xSemaphoreGive(remote_button_mutex);

            board_T01_process_remote_button(current_remote_button);
        }

        LOG("Board : feed watchdog\r\n");
        config.refresh_watchdog_callback();
    }
    return;
}


void board_init_logger ()
{
    int exit_code = board_uart_2_init(NULL);

    logger_config_t logger_config;
    logger_config.write_array_callback = board_print_uart_2;

    if (exit_code != STD_SUCCESS)
    {
        logger_config.write_array_callback = NULL;
    }

    logger_init(&logger_config);

    return;
}

void board_init_filesystem ()
{
    std_error_t error;
    std_error_init(&error);

    LOG("Board : Init SPI_1\r\n");

    board_spi_1_init_select_gpio();
    int exit_code = board_spi_1_init(&error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Board : %s\r\n", error.text);

        return;
    }

    LOG("Board : Init Flash (W25Q32BV)\r\n");
    
    w25q32bv_flash_config_t w25q32bv_flash_config;
    w25q32bv_flash_config.spi_select_callback   = board_w25q32bv_spi_1_select;
    w25q32bv_flash_config.spi_unselect_callback = board_w25q32bv_spi_1_unselect;
    w25q32bv_flash_config.spi_tx_rx_callback    = board_spi_1_read_write;
    w25q32bv_flash_config.delay_callback        = vTaskDelay;

    w25q32bv_flash_init(&w25q32bv_flash, &w25q32bv_flash_config);

    exit_code = w25q32bv_flash_release_power_down(&w25q32bv_flash, &error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Board : %s\r\n", error.text);

        return;
    }

    w25q32bv_flash_info_t flash_info;
    exit_code = w25q32bv_flash_read_info(&w25q32bv_flash, &flash_info, &error);

    if (exit_code != STD_FAILURE)
    {
        LOG("Board : Flash JEDEC ID : 0x%lX\r\n", flash_info.jedec_id);
        LOG("Board : Flash capacity: %lu KBytes\r\n", flash_info.capacity_KByte);
    }
    else
    {
        LOG("%s\r\n", error.text);
    }

    LOG("Board : Init LittleFS\r\n");

    struct lfs_config lfs_config = { 0 };
    board_lfs_init_config(&lfs_config);

    lfs_t lfs;
    int lfs_error = lfs_mount(&lfs, &lfs_config);

    if (lfs_error != 0)
    {
        LOG("Board : Format LittleFS\r\n");

        lfs_format(&lfs, &lfs_config);
        lfs_error = lfs_mount(&lfs, &lfs_config);
    }

    if (lfs_error == 0)
    {
        LOG("Board : Mount LittleFS success\r\n");
    }
    lfs_unmount(&lfs);

    exit_code = w25q32bv_flash_power_down(&w25q32bv_flash, &error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Board : %s\r\n", error.text);
    }

    return;
}

void board_init_tcp_client ()
{
    std_error_t error;
    std_error_init(&error);

    LOG("Board : Init EXTI_1\r\n");

    int exit_code = board_exti_1_init(tcp_client_ISR, &error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Board : %s\r\n", error.text);

        return;
    }

    LOG("Board : Init TCP-Client\r\n");

    tcp_client_config_t config = { 0 };

    config.process_msg_callback = node_receive_tcp_msg;

    config.spi_lock_callback        = board_spi_1_lock;
    config.spi_unlock_callback      = board_spi_1_unlock;
    config.spi_select_callback      = board_w5500_spi_1_select;
    config.spi_unselect_callback    = board_w5500_spi_1_unselect;
    config.spi_read_callback        = board_spi_1_read;
    config.spi_write_callback       = board_spi_1_write;

    config.mac_address[0] = 0xEA;
    config.mac_address[1] = 0x11;
    config.mac_address[2] = 0x22;
    config.mac_address[3] = 0x33;
    config.mac_address[4] = 0x44;
    config.mac_address[5] = 0xEA;

    config.ip_address[0] = 192;
    config.ip_address[1] = 168;
    config.ip_address[2] = 0;
    config.ip_address[3] = 123;

    config.netmask[0] = 255;
    config.netmask[1] = 255;
    config.netmask[2] = 0;
    config.netmask[3] = 0;

    config.server_ip[0] = 192;
    config.server_ip[1] = 168;
    config.server_ip[2] = 0;
    config.server_ip[3] = 101;

    config.server_port = 2399;

    if (tcp_client_init(&config, &error) != STD_SUCCESS)
    {
        LOG("Board : %s\r\n", error.text);
    }
    return;
}

void board_init_expander ()
{
    std_error_t error;
    std_error_init(&error);

    LOG("Board : Init I2C_1\r\n");

    int exit_code = board_i2c_1_init(&error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Board : %s\r\n", error.text);

        return;
    }

    LOG("Board : Init Expander (MCP23017)\r\n");

    mcp23017_expander_config_t config;
    config.write_i2c_callback   = board_i2c_1_write_register;
    config.read_i2c_callback    = board_i2c_1_read_register;
    config.i2c_timeout_ms       = I2C_TIMEOUT_MS;

    if (mcp23017_expander_init(&mcp23017_expander, &config, &error) != STD_SUCCESS)
    {
        LOG("Board : %s\r\n", error.text);
    }

    if (mcp23017_expander_set_port_direction(&mcp23017_expander, PORT_A, OUTPUT_DIRECTION, &error) != STD_SUCCESS)
    {
        LOG("Board : %s\r\n", error.text);
    }
    if (mcp23017_expander_set_port_out(&mcp23017_expander, PORT_A, LOW_GPIO, &error) != STD_SUCCESS)
    {
        LOG("Board : %s\r\n", error.text);
    }

    if (mcp23017_expander_set_port_direction(&mcp23017_expander, PORT_B, OUTPUT_DIRECTION, &error) != STD_SUCCESS)
    {
        LOG("Board : %s\r\n", error.text);
    }
    if (mcp23017_expander_set_port_out(&mcp23017_expander, PORT_B, LOW_GPIO, &error) != STD_SUCCESS)
    {
        LOG("Board : %s\r\n", error.text);
    }

    return;
}

void board_init_remote_control ()
{
    std_error_t error;
    std_error_init(&error);

    LOG("Board : Init Remote control (VS1838)\r\n");

    vs1838_control_config_t config;
    config.start_bit    = 1155U;
    config.one_bit      = 190U;
    config.zero_bit     = 99U;
    config.threshold    = 30U;

    vs1838_control_init(&vs1838_control, &config);

    LOG("Board : Init TIMER_2\r\n");

    board_timer_2_config_t timer_2_config;
    timer_2_config.ic_isr_callback = board_timer_2_ic_isr_callback;

    int exit_code = board_timer_2_init(&timer_2_config, &error);

    if (exit_code != STD_SUCCESS)
    {
        LOG("Board : %s\r\n", error.text);
    }

    LOG("Board : Init TIMER_3\r\n");

    exit_code = board_timer_3_init(&error);
    board_timer_3_deinit();

    if (exit_code != STD_SUCCESS)
    {
        LOG("Board : %s\r\n", error.text);
    }
    return;
}


void board_print_uart_2 (const uint8_t *data, uint16_t data_size)
{
    board_uart_2_write(data, data_size, 10U * 1000U, NULL);

    return;
}


void board_spi_1_init_select_gpio ()
{
    // Init 'Slave Select' GPIO ports
    // GPIO Ports Clock Enable
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // Configure GPIO pin
    // PA4 - W25Q Slave Select
    // PC13 - W5500 Slave Select
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin     = GPIO_PIN_4;
    GPIO_InitStruct.Mode    = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull    = GPIO_NOPULL;
    GPIO_InitStruct.Speed   = GPIO_SPEED_FREQ_MEDIUM;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin     = GPIO_PIN_13;
    GPIO_InitStruct.Mode    = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull    = GPIO_NOPULL;
    GPIO_InitStruct.Speed   = GPIO_SPEED_FREQ_MEDIUM;

    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    return;
}

void board_w25q32bv_spi_1_select ()
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

    return;
}

void board_w25q32bv_spi_1_unselect ()
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

    return;
}

void board_w5500_spi_1_select ()
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

    return;
}

void board_w5500_spi_1_unselect ()
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    return;
}

void board_spi_1_lock ()
{
    return;
}

void board_spi_1_unlock ()
{
    return;
}

void board_i2c_1_lock ()
{
    return;
}

void board_i2c_1_unlock ()
{
    return;
}


void board_timer_2_ic_isr_callback (uint32_t captured_value)
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
        [DOWN_BUTTON]   = DOWN_BUTTON_CODE,
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

void board_update_status_led (board_led_color_t led_color)
{
    board_led_color_t current_led_color;

    xSemaphoreTake(status_led_mutex, portMAX_DELAY);
    current_led_color = status_led_color;
    xSemaphoreGive(status_led_mutex);

    if (led_color != current_led_color)
    {
        LOG("Board : update status led\r\n");

        xSemaphoreTake(status_led_mutex, portMAX_DELAY);
        status_led_color = led_color;
        xSemaphoreGive(status_led_mutex);

        xTaskNotify(task, STATUS_LED_NOTIFICATION, eSetBits);
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

    std_error_t error;
    std_error_init(&error);

    bool is_timer_started = false;

    bool is_lightning_on;
    board_T01_get_lightning_status(&is_lightning_on);

    if (is_lightning_on != true)
    {
        if (board_set_status_led_color(NO_COLOR, &error) == STD_SUCCESS)
        {
            vTaskDelay(1U * 1000U);

            uint32_t divider_adc = 0U;

            if (board_read_photoresistor(&divider_adc, &error) != STD_FAILURE)
            {
                const uint32_t adc_max_value        = 4095U;
                const float supply_voltage_V        = 3.3F;
                const float divider_resistance_Ohm  = 10000.0F;

                photoresistor_data_t data;
                data.adc        = adc_max_value - divider_adc;
                data.voltage_V  = supply_voltage_V * ((float)(data.adc) / (float)(adc_max_value));

                const float divider_voltage_V = supply_voltage_V - data.voltage_V;
                const float current_A = divider_voltage_V / divider_resistance_Ohm;

                data.resistance_Ohm = (uint32_t)(supply_voltage_V / current_A);

                LOG("Board : photoresistor adc = %lu\r\n", data.adc);
                LOG("Board : photoresistor voltage = %.2f V\r\n", data.voltage_V);
                LOG("Board : photoresistor resistance = %lu Ohm\r\n", data.resistance_Ohm);

                uint32_t timer_period_ms;
                board_T01_process_photoresistor_data(&data, &timer_period_ms);

                xTimerChangePeriod(photoresistor_timer, timer_period_ms / portTICK_PERIOD_MS, RTOS_TIMER_TICKS_TO_WAIT);
            }
            else
            {
                LOG("Board : %s\r\n", error.text);
            }
        }
        else
        {
            LOG("Board : %s\r\n", error.text);
        }
    }

    if (is_timer_started != true)
    {
        xTimerStart(photoresistor_timer, RTOS_TIMER_TICKS_TO_WAIT);
    }

    xTaskNotify(task, STATUS_LED_NOTIFICATION, eSetBits);

    return;
}

int board_read_photoresistor (uint32_t * const photoresistor_adc, std_error_t * const error)
{
    int exit_code = board_adc_1_init(error);

    if (exit_code == STD_SUCCESS)
    {
        uint32_t photoresistor_buffer       = 0U;
        uint32_t photoresistor_buffer_size  = 0U;

        for (size_t i = 0U; i < PHOTORESISTOR_MEAUSEREMENT_COUNT; ++i)
        {
            uint32_t adc_value;
            exit_code = board_adc_1_read_value(&adc_value, PHOTORESISTOR_MEAUSEREMENT_TIMEOUT_MS, error);

            if (exit_code == STD_SUCCESS)
            {
                photoresistor_buffer += adc_value;
                ++photoresistor_buffer_size;
            }
        }

        if (photoresistor_buffer_size != 0U)
        {
            *photoresistor_adc = photoresistor_buffer / photoresistor_buffer_size;
        }
        else
        {
            exit_code = STD_FAILURE;
        }
    }
    board_adc_1_deinit();

    return exit_code;
}

int board_malloc (std_error_t * const error)
{
    status_led_mutex    = xSemaphoreCreateMutex();
    remote_button_mutex = xSemaphoreCreateMutex();

    const bool are_semaphores_allocated = (status_led_mutex != NULL) && (remote_button_mutex != NULL);

    photoresistor_timer = xTimerCreate("photoresistor", (PHOTORESISTOR_INITIAL_PERIOD_MS / portTICK_PERIOD_MS), pdFALSE, NULL, board_photoresistor_timer);

    const bool are_timers_allocated = (photoresistor_timer != NULL);

    if ((are_semaphores_allocated != true) || (are_timers_allocated != true))
    {
        vSemaphoreDelete(status_led_mutex);
        vSemaphoreDelete(remote_button_mutex);
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



int board_lfs_block_device_read (const struct lfs_config *config,
                                lfs_block_t sector_number,
                                lfs_off_t sector_offset,
                                void *raw_data,
                                lfs_size_t size)
{
    const w25q32bv_flash_t *flash = (const w25q32bv_flash_t *)config->context;

    const int exit_code = w25q32bv_flash_read_data_fast(flash, (uint8_t*)raw_data, (uint32_t)size, (uint32_t)sector_number, (uint32_t)sector_offset, NULL);

    return exit_code;
}

int board_lfs_block_device_prog (const struct lfs_config *config,
                                lfs_block_t sector_number,
                                lfs_off_t sector_offset,
                                const void *raw_data,
                                lfs_size_t size)
{
    int exit_code = STD_FAILURE;

    const w25q32bv_flash_t *flash = (const w25q32bv_flash_t *)config->context;

    uint32_t page_number = (((uint32_t)(sector_number)*flash->sector_size) / flash->page_size) + ((uint32_t)(sector_offset) / flash->page_size);
    uint32_t page_offset = (uint32_t)(sector_offset) % flash->page_size;

    uint8_t *data = (uint8_t *)raw_data;
    uint32_t data_size = (uint32_t)size;

    const uint32_t page_free_size = flash->page_size - page_offset;
    uint32_t size_to_write = data_size;

    if (data_size > page_free_size)
    {
        size_to_write = page_free_size;
    }

    while (true)
    {
        exit_code = w25q32bv_flash_enable_erasing_or_writing(flash, NULL);

        if (exit_code != STD_SUCCESS)
        {
            return exit_code;
        }

        exit_code = w25q32bv_flash_write_page(flash, data, size_to_write, page_number, page_offset, NULL);

        if (exit_code != STD_SUCCESS)
        {
            return exit_code;
        }

        exit_code = w25q32bv_flash_wait_erasing_or_writing(flash, NULL);

        if (exit_code != STD_SUCCESS)
        {
            return exit_code;
        }

        if (data_size > size_to_write)
        {
            ++page_number;
            page_offset = 0U;
            data += (uint8_t)(size_to_write);
            data_size -= size_to_write;

            if (data_size > flash->page_size)
            {
                size_to_write = flash->page_size;
            }
            else
            {
                size_to_write = data_size;
            }
        }
        else
        {
            break;
        }
    }

    return exit_code;
}

int board_lfs_block_device_erase (const struct lfs_config *config, lfs_block_t sector_number)
{
    const w25q32bv_flash_t *flash = (w25q32bv_flash_t *)config->context;

    int exit_code = w25q32bv_flash_enable_erasing_or_writing(flash, NULL);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    exit_code = w25q32bv_flash_erase_sector(flash, (uint32_t)sector_number, NULL);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    exit_code = w25q32bv_flash_wait_erasing_or_writing(flash, NULL);

    return exit_code;
}

int board_lfs_block_device_sync (const struct lfs_config *config)
{
    return STD_SUCCESS;
}

int board_lfs_block_device_lock (const struct lfs_config *config)
{
    return STD_SUCCESS;
}

int board_lfs_block_device_unlock (const struct lfs_config *config)
{
    return STD_SUCCESS;
}

void board_lfs_init_config (struct lfs_config *lfs_config)
{
    // Block device operations
    lfs_config->read    = board_lfs_block_device_read;
    lfs_config->prog    = board_lfs_block_device_prog;
    lfs_config->erase   = board_lfs_block_device_erase;
    lfs_config->sync    = board_lfs_block_device_sync;
    lfs_config->lock    = board_lfs_block_device_lock;
    lfs_config->unlock  = board_lfs_block_device_unlock;
    lfs_config->context = (void*)&w25q32bv_flash;
    
    // Block device configuration
    lfs_config->read_size       = LFS_MIN_READ_BLOCK_SIZE;
    lfs_config->prog_size       = LFS_MIN_PROG_BLOCK_SIZE;
    lfs_config->block_size      = w25q32bv_flash.sector_size;
    lfs_config->block_count     = w25q32bv_flash.sector_count;
    lfs_config->cache_size      = LFS_CACHE_SIZE;
    lfs_config->lookahead_size  = LFS_LOOKAHEAD_SIZE;
    lfs_config->block_cycles    = LFS_ERASE_CYCLES;

    lfs_config->read_buffer         = lfs_read_buffer;
    lfs_config->prog_buffer         = lfs_prog_buffer;
    lfs_config->lookahead_buffer    = lfs_lookahead_buffer;

    return;
}
