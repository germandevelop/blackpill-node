/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef BOARD_I2C_1_H
#define BOARD_I2C_1_H

#include <stdint.h>

typedef struct std_error std_error_t;

int board_i2c_1_init (std_error_t * const error);
void board_i2c_1_deinit ();

void board_i2c_1_enable_clock ();
void board_i2c_1_disable_clock ();

int board_i2c_1_write_register (uint16_t device_address,
                                uint16_t register_address,
                                uint16_t register_size,
                                uint8_t *array,
                                uint16_t array_size,
                                uint32_t timeout_ms,
                                std_error_t * const error);

int board_i2c_1_read_register (uint16_t device_address,
                                uint16_t register_address,
                                uint16_t register_size,
                                uint8_t *array,
                                uint16_t array_size,
                                uint32_t timeout_ms,
                                std_error_t * const error);

int board_i2c_1_write (uint16_t device_address,
                        uint8_t *array,
                        uint16_t array_size,
                        uint32_t timeout_ms,
                        std_error_t * const error);

#endif // BOARD_I2C_1_H
