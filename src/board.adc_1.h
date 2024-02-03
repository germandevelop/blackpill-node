/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef BOARD_ADC_1_H
#define BOARD_ADC_1_H

#include <stdint.h>

typedef struct std_error std_error_t;

int board_adc_1_init (std_error_t * const error);
void board_adc_1_deinit ();

void board_adc_1_enable_clock ();
void board_adc_1_disable_clock ();

int board_adc_1_read_value (uint32_t * const adc_value, uint32_t timeout_ms, std_error_t * const error);

#endif // BOARD_ADC_1_H
