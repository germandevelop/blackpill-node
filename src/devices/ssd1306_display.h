/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef SSD1306_DISPLAY_H
#define SSD1306_DISPLAY_H

#include <stdint.h>
#include <stddef.h>

#define SSD1306_DISPLAY_WIDTH_PX    128U
#define SSD1306_DISPLAY_HEIGHT_PX   64U

#define SSD1306_DISPLAY_PIXEL_BUFFER_SIZE   ((SSD1306_DISPLAY_WIDTH_PX * SSD1306_DISPLAY_HEIGHT_PX) / 8U)
#define SSD1306_DISPLAY_ADDRESS_1           0x3C
#define SSD1306_DISPLAY_ADDRESS_2           0x3D

typedef struct std_error std_error_t;

typedef int (*ssd1306_display_i2c_callback_t) (uint16_t device_address, uint8_t *array, uint16_t array_size, uint32_t timeout_ms, std_error_t * const error);
typedef int (*ssd1306_display_i2c_dma_callback_t) (uint16_t device_address, uint8_t *array, uint16_t array_size, std_error_t * const error);

typedef struct ssd1306_display_config
{
    ssd1306_display_i2c_callback_t write_i2c_callback;
    ssd1306_display_i2c_dma_callback_t write_i2c_dma_callback; // Optional (might be 'NULL')
    uint32_t i2c_timeout_ms;

    uint8_t *pixel_buffer;
    uint16_t device_address;

} ssd1306_display_config_t;

typedef struct ssd1306_display ssd1306_display_t;


int ssd1306_display_init (  ssd1306_display_t * const self,
                            ssd1306_display_config_t const * const init_config,
                            std_error_t * const error);

void ssd1306_display_reset_buffer (ssd1306_display_t * const self);

void ssd1306_display_draw_text_10x16 (  ssd1306_display_t * const self,
                                        char const * const text,
                                        size_t text_length,
                                        uint8_t X,
                                        uint8_t Y,
                                        uint8_t * const X_shift);

void ssd1306_display_draw_text_16x26 (  ssd1306_display_t * const self,
                                        char const * const text,
                                        size_t text_length,
                                        uint8_t X,
                                        uint8_t Y,
                                        uint8_t * const X_shift);

int ssd1306_display_update_full_screen (ssd1306_display_t * const self, std_error_t * const error);


// Private
typedef struct ssd1306_display
{
    ssd1306_display_config_t config;

} ssd1306_display_t;

#endif // SSD1306_DISPLAY_H
