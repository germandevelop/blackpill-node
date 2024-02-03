/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef BOARD_SPI_1_H
#define BOARD_SPI_1_H

#include <stdint.h>

typedef struct std_error std_error_t;

int board_spi_1_init (std_error_t * const error);
void board_spi_1_deinit ();

void board_spi_1_enable_clock ();
void board_spi_1_disable_clock ();

int board_spi_1_read (uint8_t *data,
                        uint16_t size,
                        uint32_t timeout_ms,
                        std_error_t * const error);

int board_spi_1_write (uint8_t *data,
                        uint16_t size,
                        uint32_t timeout_ms,
                        std_error_t * const error);

int board_spi_1_read_write (uint8_t *tx_data,
                            uint8_t *rx_data,
                            uint16_t size,
                            uint32_t timeout_ms,
                            std_error_t * const error);

#endif // BOARD_SPI_1_H
