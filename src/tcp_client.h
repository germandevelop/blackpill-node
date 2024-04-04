/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <stdint.h>

typedef struct tcp_msg tcp_msg_t;
typedef struct std_error std_error_t;

typedef void (*tcp_client_spi_lock_callback_t) ();
typedef void (*tcp_client_spi_select_callback_t) ();
typedef int (*tcp_client_spi_tx_rx_callback_t) (uint8_t *data, uint16_t size, uint32_t timeout_ms, std_error_t * const error);
typedef int (*tcp_client_process_msg_callback_t) (tcp_msg_t const * const recv_msg, std_error_t * const error);

typedef struct tcp_client_config
{
    uint8_t mac_address[6];
    uint8_t ip_address[4];
    uint8_t netmask[4];

    uint8_t server_ip[4];
    uint16_t server_port;

    tcp_client_process_msg_callback_t process_msg_callback;

    tcp_client_spi_lock_callback_t spi_lock_callback;
    tcp_client_spi_lock_callback_t spi_unlock_callback;
    tcp_client_spi_select_callback_t spi_select_callback;
    tcp_client_spi_select_callback_t spi_unselect_callback;
    tcp_client_spi_tx_rx_callback_t spi_read_callback;
    tcp_client_spi_tx_rx_callback_t spi_write_callback;
    uint32_t spi_timeout_ms;

} tcp_client_config_t;

int tcp_client_init (tcp_client_config_t const * const init_config, std_error_t * const error);
void tcp_client_send_message (tcp_msg_t const * const send_msg);

void tcp_client_ISR ();

#endif // TCP_CLIENT_H
