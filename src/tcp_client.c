/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "tcp_client.h"

#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "socket.h"

#include "embedded_logger.h"
#include "std_error/std_error.h"


#define RTOS_TASK_STACK_SIZE    512U            // 512*4=2048 bytes
#define RTOS_TASK_PRIORITY      4U              // 0 - lowest, 4 - highest
#define RTOS_TASK_NAME          "tcp_client"    // 16 - max length

#define W5500_SOCKET_NUMBER 0U

#define SEND_BUFFER_SIZE 256U
#define RECV_BUFFER_SIZE 1024U

#define DEFAULT_ERROR_TEXT  "TCP-Client error"
#define MALLOC_ERROR_TEXT   "TCP-Client memory allocation error"

#define SOCKET_INTERRUPT_NOTIFICATION   (1 << 0)
#define SEND_MESSAGE_NOTIFICATION       (1 << 1)

#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


#ifdef NDEBUG
#define TCP_DEBUG(...) ((void)0U)
#else
//static void tcp_client_print_state (char *header);
//#define TCP_DEBUG(...) tcp_client_print_state(__VA_ARGS__)
#define TCP_DEBUG(...) ((void)0U)
#endif // NDEBUG


static TaskHandle_t task;
static SemaphoreHandle_t send_mutex;

static tcp_client_config_t config;
static tcp_msg_t send_msg;
static tcp_msg_t recv_msg;


static void tcp_client_spi_lock ();
static void tcp_client_spi_unlock ();
static void tcp_client_spi_select ();
static void tcp_client_spi_unselect ();
static void tcp_client_spi_read_data (uint8_t *data, uint16_t size);
static void tcp_client_spi_write_data (uint8_t *data, uint16_t size);
static uint8_t tcp_client_spi_read_byte ();
static void tcp_client_spi_write_byte (uint8_t byte);

static int tcp_client_malloc (std_error_t * const error);
static void tcp_client_task (void *parameters);

static int tcp_client_setup_w5500 (std_error_t * const error);
static int tcp_client_connect (std_error_t * const error);

int tcp_client_init (tcp_client_config_t const * const init_config, std_error_t * const error)
{
    assert(init_config                          != NULL);
    assert(init_config->spi_lock_callback       != NULL);
    assert(init_config->spi_unlock_callback     != NULL);
    assert(init_config->spi_select_callback     != NULL);
    assert(init_config->spi_unselect_callback   != NULL);
    assert(init_config->spi_read_callback       != NULL);
    assert(init_config->spi_write_callback      != NULL);

    memcpy((void*)(&config), (const void*)(init_config), sizeof(tcp_client_config_t));

    send_msg.size = 0U;
    recv_msg.size = 0U;

    return tcp_client_malloc(error);
}

void tcp_client_ISR ()
{
    BaseType_t is_higher_priority_task_woken;

    xTaskNotifyFromISR(task, SOCKET_INTERRUPT_NOTIFICATION, eSetBits, &is_higher_priority_task_woken);

    portYIELD_FROM_ISR(is_higher_priority_task_woken);

    return;
}

void tcp_client_send_message (tcp_client_serialize_msg_callback_t serialize_msg_callback, void *user_msg)
{
    assert(serialize_msg_callback != NULL);
    assert(user_msg != NULL);

    xSemaphoreTake(send_mutex, portMAX_DELAY);

    serialize_msg_callback(user_msg, &send_msg);

    xSemaphoreGive(send_mutex);

    xTaskNotify(task, SEND_MESSAGE_NOTIFICATION, eSetBits);

    return;
}

void tcp_client_task (void *parameters)
{
    UNUSED(parameters);
    
    std_error_t error;
    std_error_init(&error);

    while (true)
    {
        if (tcp_client_setup_w5500(&error) != STD_SUCCESS)
        {
            LOG("%s\r\n", error.text);

            vTaskDelay(5U * 1000U);
        }
        else
        {
            break;
        }
    }
    vTaskDelay(5U * 1000U);

    bool is_connected = false;

    xTaskNotify(task, SOCKET_INTERRUPT_NOTIFICATION, eSetBits);

    while (true)
    {
        uint32_t notification;
        xTaskNotifyWait(0U, ULONG_MAX, &notification, 30U * 1000U);

        if ((notification & SEND_MESSAGE_NOTIFICATION) != 0U)
        {
            LOG("TCP-Client: try to send message\r\n");

            xSemaphoreTake(send_mutex, portMAX_DELAY);

            const int32_t exit_code = send(W5500_SOCKET_NUMBER, send_msg.buffer, send_msg.size);

            xSemaphoreGive(send_mutex);

            if (exit_code < SOCK_OK)
            {
                LOG("TCP-Client: message sending is failed %li\r\n", exit_code);
            }
        }

        if ((notification & SOCKET_INTERRUPT_NOTIFICATION) != 0U)
        {
            uint8_t interrupt_kind;
            ctlsocket(W5500_SOCKET_NUMBER, CS_GET_INTERRUPT, (void*)(&interrupt_kind));

            uint8_t clear_interrupt = (uint8_t)(SIK_RECEIVED | SIK_DISCONNECTED);
            ctlsocket(W5500_SOCKET_NUMBER, CS_CLR_INTERRUPT, (void*)(&clear_interrupt));

            LOG("TCP-Client: ISR - %u\r\n", interrupt_kind);

            if ((interrupt_kind & (uint8_t)(SIK_RECEIVED)) != 0U)
            {
                LOG("TCP-Client: ISR - SIK_RECEIVED\r\n");

                uint8_t msg_buffer[256] = { 0 };
                int32_t msg_size = recv(W5500_SOCKET_NUMBER, msg_buffer, 256);

                if (msg_size > 0)
                {
                    LOG("TCP-Client: input message - \'%s\'\r\n", msg_buffer);
                }
                else
                {
                    LOG("TCP-Client: input message is empty\r\n");
                }
            }

            if ((interrupt_kind & (uint8_t)(SIK_DISCONNECTED)) != 0U)
            {
                LOG("TCP-Client: ISR - SIK_DISCONNECTED\r\n");

                is_connected = false;

                // Disable interrupts
                uint8_t clear_interrupt_mask = 0U;
                ctlsocket(W5500_SOCKET_NUMBER, CS_SET_INTMASK, (void*)(&clear_interrupt_mask));
            }
        }

        const int8_t phy_link = wizphy_getphylink();

        if (phy_link != PHY_LINK_ON)
        {
            is_connected = false;
        }

        if (is_connected != true)
        {
            while (true)
            {
                if (tcp_client_connect(&error) != STD_SUCCESS)
                {
                    LOG("TCP-Client: Connection fail\r\n");

                    vTaskDelay(10U * 1000U);
                }
                else
                {
                    LOG("TCP-Client: Connection STD_SUCCESS\r\n");

                    is_connected = true;

                    break;
                }
            }
        }
    }

    return;
}

int tcp_client_connect (std_error_t * const error)
{
    const int8_t phy_link = wizphy_getphylink();
    
    if (phy_link != PHY_LINK_ON)
    {
        std_error_catch_custom(error, (int)phy_link, DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    TCP_DEBUG("try to disconnect");

    uint8_t socket_status;
    getsockopt(W5500_SOCKET_NUMBER, SO_STATUS, (void*)(&socket_status));

    if (socket_status == SOCK_CLOSE_WAIT)
    {
        const int8_t exit_code = disconnect(W5500_SOCKET_NUMBER);

        if (exit_code != SOCK_OK)
        {
            close(W5500_SOCKET_NUMBER);
        }
    }

    TCP_DEBUG("try to create a socket");

    int8_t exit_code = socket(W5500_SOCKET_NUMBER, Sn_MR_TCP, 0U, 0U);

    if (exit_code != W5500_SOCKET_NUMBER)
    {
        std_error_catch_custom(error, (int)exit_code, DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    TCP_DEBUG("try to connect");


    exit_code = connect(W5500_SOCKET_NUMBER, config.server_ip, config.server_port);

    if (exit_code != SOCK_OK)
    {
        std_error_catch_custom(error, (int)exit_code, DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    else
    {
        uint8_t socket_interrupt_mask = (uint8_t)(SIK_DISCONNECTED | SIK_RECEIVED);

        ctlsocket(W5500_SOCKET_NUMBER, CS_SET_INTMASK, (void*)(&socket_interrupt_mask));
    }

    return STD_SUCCESS;
}

int tcp_client_setup_w5500 (std_error_t * const error)
{
    TCP_DEBUG("setup begin");

    reg_wizchip_cris_cbfunc(tcp_client_spi_lock, tcp_client_spi_unlock);
    reg_wizchip_cs_cbfunc(tcp_client_spi_select, tcp_client_spi_unselect);
    reg_wizchip_spi_cbfunc(tcp_client_spi_read_byte, tcp_client_spi_write_byte);
    reg_wizchip_spiburst_cbfunc(tcp_client_spi_read_data, tcp_client_spi_write_data);

    uint8_t rx_tx_buffer_sizes[8] = { 0 };
    rx_tx_buffer_sizes[W5500_SOCKET_NUMBER] = 16U;

    int8_t exit_code = wizchip_init(rx_tx_buffer_sizes, rx_tx_buffer_sizes);

    if (exit_code != 0)
    {
        std_error_catch_custom(error, (int)exit_code, DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    wiz_PhyConf phy_config;
    phy_config.by       = PHY_CONFBY_SW;
    phy_config.mode     = PHY_MODE_MANUAL;
    phy_config.duplex   = PHY_DUPLEX_FULL;
    phy_config.speed    = PHY_SPEED_10;

    wizphy_setphyconf(&phy_config);

    wiz_NetTimeout timeout_config;
    timeout_config.time_100us   = 2000U;
    timeout_config.retry_cnt    = 8U;

    wizchip_settimeout(&timeout_config);

    wiz_NetInfo net_info;
    memcpy((void*)(net_info.mac), (const void*)(config.mac_address), sizeof(net_info.mac));
    memcpy((void*)(net_info.ip), (const void*)(config.ip_address), sizeof(net_info.ip));
    memcpy((void*)(net_info.sn), (const void*)(config.netmask), sizeof(net_info.sn));
    net_info.dhcp = NETINFO_STATIC;

    wizchip_setnetinfo(&net_info);

    wizchip_setinterruptmask(IK_SOCK_0);

    //wizchip_setnetmode(netmode_type netmode); // Unknown
    //wizphy_setphypmode(PHY_POWER_DOWN);

    TCP_DEBUG("setup end");

    return STD_SUCCESS;
}

int tcp_client_malloc (std_error_t * const error)
{
    send_msg.buffer = (uint8_t*)pvPortMalloc(SEND_BUFFER_SIZE * sizeof(uint8_t));
    recv_msg.buffer = (uint8_t*)pvPortMalloc(RECV_BUFFER_SIZE * sizeof(uint8_t));

    const bool are_buffers_allocated = (send_msg.buffer != NULL) && (recv_msg.buffer != NULL);

    send_mutex = xSemaphoreCreateMutex();

    const bool are_semaphores_allocated = (send_mutex != NULL);

    if ((are_buffers_allocated != true) || (are_semaphores_allocated != true))
    {
        vPortFree((void*)send_msg.buffer);
        vPortFree((void*)recv_msg.buffer);

        vSemaphoreDelete(send_mutex);

        std_error_catch_custom(error, STD_FAILURE, MALLOC_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    BaseType_t exit_code = xTaskCreate(tcp_client_task, RTOS_TASK_NAME, RTOS_TASK_STACK_SIZE, NULL, RTOS_TASK_PRIORITY, &task);

    if (exit_code != pdPASS)
    {
        std_error_catch_custom(error, (int)exit_code, MALLOC_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}


void tcp_client_spi_lock ()
{
    config.spi_lock_callback();

    return;
}

void tcp_client_spi_unlock ()
{
    config.spi_unlock_callback();

    return;
}

void tcp_client_spi_select ()
{
    config.spi_select_callback();

    return;
}

void tcp_client_spi_unselect ()
{
    config.spi_unselect_callback();

    return;
}

void tcp_client_spi_read_data (uint8_t *data, uint16_t size)
{
    config.spi_read_callback(data, size, 8U * 1000U, NULL);

    return;
}

void tcp_client_spi_write_data (uint8_t *data, uint16_t size)
{
    config.spi_write_callback(data, size, 8U * 1000U, NULL);

    return;
}

uint8_t tcp_client_spi_read_byte ()
{
    uint8_t byte;

    config.spi_read_callback(&byte, sizeof(byte), 2U * 1000U, NULL);

    return byte;
}

void tcp_client_spi_write_byte (uint8_t byte)
{
    config.spi_write_callback(&byte, sizeof(byte), 2U * 1000U, NULL);

    return;
}


#ifdef NDEBUG
void tcp_client_print_state (char *header)
{
    LOG("\r\n___W5500 - %s\r\n", header);

    int8_t phylink = wizphy_getphylink();
    int8_t phymode = wizphy_getphypmode();

    LOG("link: %i | mode: %i\r\n", phylink, phymode);

    wiz_PhyConf phyconf;
    wizphy_getphyconf(&phyconf);

    LOG("by: %u | duplex: %u | mode: %u | speed: %u\r\n", phyconf.by, phyconf.duplex, phyconf.mode, phyconf.speed);

    wizphy_getphystat(&phyconf);

    LOG("duplex: %u | speed: %u\r\n", phyconf.duplex, phyconf.speed);

    netmode_type type = wizchip_getnetmode();

    LOG("type: %i\r\n", type);
    LOG("NM_FORCEARP - %i; NM_WAKEONLAN - %i; NM_PINGBLOCK - %i; NM_PPPOE - %i\r\n", NM_FORCEARP, NM_WAKEONLAN, NM_PINGBLOCK, NM_PPPOE);

    wiz_NetTimeout nettime;
    wizchip_gettimeout(&nettime);

    LOG("retry: %u | time: %u\r\n", nettime.retry_cnt, nettime.time_100us);

    intr_kind mask_low = wizchip_getinterruptmask();

    LOG("interrupt mask: %i\r\n", mask_low);

    uint8_t mask;
    ctlsocket(W5500_SOCKET_NUMBER, CS_GET_INTMASK, (void*)&mask);

    LOG("socket int mask: %u\r\n", mask);
    LOG("SIK_CONNECTED - %u; SIK_DISCONNECTED - %u; SIK_RECEIVED - %u\r\n", SIK_CONNECTED, SIK_DISCONNECTED, SIK_RECEIVED);
    LOG("SIK_TIMEOUT - %u; SIK_SENT - %u; SIK_ALL - %u\r\n", SIK_TIMEOUT, SIK_SENT, SIK_ALL);
        
    uint8_t status;
    getsockopt(W5500_SOCKET_NUMBER, SO_STATUS, (void*)(&status));

    if (status == SOCK_CLOSED) LOG("status: SOCK_CLOSED\r\n");
    else if (status == SOCK_INIT) LOG("status: SOCK_INIT\r\n");
    else if (status == SOCK_LISTEN) LOG("status: SOCK_LISTEN\r\n");
    else if (status == SOCK_SYNSENT) LOG("status: SOCK_SYNSENT\r\n");
    else if (status == SOCK_SYNRECV) LOG("status: SOCK_SYNRECV\r\n");
    else if (status == SOCK_ESTABLISHED) LOG("status: SOCK_ESTABLISHED\r\n");
    else if (status == SOCK_FIN_WAIT) LOG("status: SOCK_FIN_WAIT\r\n");
    else if (status == SOCK_CLOSING) LOG("status: SOCK_CLOSING\r\n");
    else if (status == SOCK_TIME_WAIT) LOG("status: SOCK_TIME_WAIT\r\n");
    else if (status == SOCK_CLOSE_WAIT) LOG("status: SOCK_CLOSE_WAIT\r\n");
    else if (status == SOCK_LAST_ACK) LOG("status: SOCK_LAST_ACK\r\n");
    else if (status == SOCK_UDP) LOG("status: SOCK_UDP\r\n");
    else if (status == SOCK_IPRAW) LOG("status: SOCK_IPRAW\r\n");
    else if (status == SOCK_MACRAW) LOG("status: SOCK_MACRAW\r\n");

    return;
}
#endif // NDEBUG
