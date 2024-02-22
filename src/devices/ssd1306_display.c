/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "ssd1306_display.h"

#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "std_error/std_error.h"


#define SET_DISPLAY_START_LINE_COMMAND 0x40


static void font10x16_draw_symbol (ssd1306_display_t * const self, uint8_t symbol, uint8_t X, uint8_t Y);
static void font10x16_draw_pixel (ssd1306_display_t * const self, bool is_dark_pixel, uint8_t X, uint8_t Y);
static void font10x16_get_symbol_table (uint8_t symbol, uint8_t **symbol_table);
static void font10x16_get_symbol_width (uint8_t const * const symbol_table, uint8_t * const symbol_width);
static void font10x16_get_symbol_height (uint8_t const * const symbol_table, uint8_t * const symbol_height);

static void font16x26_draw_symbol (ssd1306_display_t * const self, uint8_t symbol, uint8_t X, uint8_t Y);
static void font16x26_draw_pixel (ssd1306_display_t * const self, bool is_dark_pixel, uint16_t X, uint16_t Y);
static void font16x26_get_table (uint16_t **table);

int ssd1306_display_init (  ssd1306_display_t * const self,
                            ssd1306_display_config_t const * const init_config,
                            std_error_t * const error)
{
    assert(self                             != NULL);
    assert(init_config                      != NULL);
    assert(init_config->write_i2c_callback  != NULL);
    assert(init_config->pixel_buffer        != NULL);

    bool is_address_valid = (init_config->device_address == SSD1306_DISPLAY_ADDRESS_1) || (init_config->device_address == SSD1306_DISPLAY_ADDRESS_2);
    assert(is_address_valid == true);

    self->config = *init_config;

    uint8_t data[2];

    // Enable 'sleep' mode - display off
    data[0] = 0x00;
    data[1] = 0xAE;

    int exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Set memory addressing mode   
    data[0] = 0x00;
    data[1] = 0x20;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // 0x10 - Page Addressing Mode (RESET)
    data[0] = 0x00;
    data[1] = 0x10;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Set Page Start Address for Page Addressing Mode,0-7
    data[0] = 0x00;
    data[1] = 0xB0;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Set COM Output Scan Direction
    data[0] = 0x00;
    data[1] = 0xC8;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Set low column address
    data[0] = 0x00;
    data[1] = 0x00;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Set high column address
    data[0] = 0x00;
    data[1] = 0x10;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Set start line address
    data[0] = 0x00;
    data[1] = 0x40;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Set contrast control register
    data[0] = 0x00;
    data[1] = 0x81;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Max contrast - 255
    data[0] = 0x00;
    data[1] = 0xFF;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Set segment re-map 0 to 127
    data[0] = 0x00;
    data[1] = 0xA1;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Set normal display
    data[0] = 0x00;
    data[1] = 0xA6;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Set multiplex ratio (1 to 64)
    data[0] = 0x00;
    data[1] = 0xA8;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    data[0] = 0x00;
    data[1] = 0x3F;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // 0xA4 - Output follows
    data[0] = 0x00;
    data[1] = 0xA4;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Set display offset
    data[0] = 0x00;
    data[1] = 0xD3;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // No offset
    data[0] = 0x00;
    data[1] = 0x00;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Set display clock divide ratio/oscillator frequency
    data[0] = 0x00;
    data[1] = 0xD5;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Set divide ratio
    data[0] = 0x00;
    data[1] = 0xF0;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Set pre-charge period
    data[0] = 0x00;
    data[1] = 0xD9;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    data[0] = 0x00;
    data[1] = 0x22;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Set com pins hardware configuration
    data[0] = 0x00;
    data[1] = 0xDA;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    data[0] = 0x00;
    data[1] = 0x12;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Set vcomh
    data[0] = 0x00;
    data[1] = 0xDB;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // 0x20 - 0.77xVcc
    data[0] = 0x00;
    data[1] = 0x20;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Set DC-DC enable
    data[0] = 0x00;
    data[1] = 0x8D;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    data[0] = 0x00;
    data[1] = 0x14;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    // Wake up - display on
    data[0] = 0x00;
    data[1] = 0xAF;

    exit_code = self->config.write_i2c_callback(self->config.device_address, data, sizeof(data), self->config.i2c_timeout_ms, error);

    if (exit_code != STD_SUCCESS)
    {
        return exit_code;
    }

    return exit_code;
}

void ssd1306_display_reset_buffer (ssd1306_display_t * const self)
{
    memset(self->config.pixel_buffer, 0, SSD1306_DISPLAY_PIXEL_BUFFER_SIZE);

    return;
}

void ssd1306_display_draw_text_10x16 (  ssd1306_display_t * const self,
                                        char const * const text,
                                        size_t text_length,
                                        uint8_t X,
                                        uint8_t Y,
                                        uint8_t * const X_shift)
{
    *X_shift = 0U;

    for (size_t i = 0U; i < text_length; ++i)
    {
        font10x16_draw_symbol(self, (uint8_t)text[i], X + (*X_shift), Y);

        uint8_t *symbol_table;
        font10x16_get_symbol_table(text[i], &symbol_table);

        uint8_t symbol_width;
        font10x16_get_symbol_width(symbol_table, &symbol_width);

        *X_shift += symbol_width;
    }
    return;
}

void font10x16_draw_symbol (ssd1306_display_t * const self, uint8_t symbol, uint8_t X, uint8_t Y)
{
    uint8_t *symbol_table;
    font10x16_get_symbol_table(symbol, &symbol_table);

    uint8_t symbol_height;
    font10x16_get_symbol_height(symbol_table, &symbol_height);

    uint8_t symbol_width;
    font10x16_get_symbol_width(symbol_table, &symbol_width);

    symbol_table += 2U;

    for (uint8_t row = 0U; row < symbol_height; ++row)
    {
        for (uint8_t col = 0U; col < symbol_width; ++col)
        {
            bool is_dark_pixel = true;

            if (col < 8U)
            {
                if ((symbol_table[row * 2U] & (1 << (7U - col))) != 0U)
                {
                    is_dark_pixel = false;
                }
            }
            else
            {
                if ((symbol_table[(row * 2U) + 1U] & (1 << (15U - col))) != 0U)
                {
                    is_dark_pixel = false;
                }
            }
            font10x16_draw_pixel(self, is_dark_pixel, X + col, Y + row);
        }
    }
    return;
}

void font10x16_draw_pixel (ssd1306_display_t * const self, bool is_dark_pixel, uint8_t X, uint8_t Y)
{
    if ((X >= SSD1306_DISPLAY_WIDTH_PX) || (Y >= SSD1306_DISPLAY_HEIGHT_PX))
    {
        return;
    }

    uint16_t byteIdx = Y >> 3;
    uint8_t bitIdx = Y - (byteIdx << 3);
    byteIdx *= SSD1306_DISPLAY_WIDTH_PX;
    byteIdx += X;

    if (is_dark_pixel == true)
    {
        self->config.pixel_buffer[byteIdx] &= ~(1 << bitIdx);
    }
    else
    {
        self->config.pixel_buffer[byteIdx] |= (1 << bitIdx);
    }
    return;
}

void ssd1306_display_draw_text_16x26 (  ssd1306_display_t * const self,
                                        char const * const text,
                                        size_t text_length,
                                        uint8_t X,
                                        uint8_t Y,
                                        uint8_t * const X_shift)
{
    *X_shift = 0U;

    const uint32_t symbol_width = 16U;

    for (size_t i = 0U; i < text_length; ++i)
    {
        font16x26_draw_symbol(self, (uint8_t)text[i], X + (*X_shift), Y);

        *X_shift += symbol_width;
    }
    return;
}

void font16x26_draw_symbol (ssd1306_display_t * const self, uint8_t symbol, uint8_t X, uint8_t Y)
{
    uint16_t *table;
    font16x26_get_table(&table);

    const uint32_t symbol_height    = 26U;
    const uint32_t symbol_width     = 16U;

    for (uint32_t row = 0U; row < symbol_height; ++row)
    {
        const uint32_t b = table[(symbol - 32U) * symbol_height + row];

        for (uint32_t col = 0U; col < symbol_width; ++col)
        {
            bool is_dark_pixel = true;

            if ((b << col) & 0x8000)
            {
                is_dark_pixel = false;
            }

            font16x26_draw_pixel(self, is_dark_pixel, X + col, Y + row);
        }
    }
    return;
}

void font16x26_draw_pixel (ssd1306_display_t * const self, bool is_dark_pixel, uint16_t X, uint16_t Y)
{
    if ((X >= SSD1306_DISPLAY_WIDTH_PX) || (Y >= SSD1306_DISPLAY_HEIGHT_PX))
    {
        return;
    }

    if (is_dark_pixel == true)
    {
        self->config.pixel_buffer[X + (Y / 8U) * SSD1306_DISPLAY_WIDTH_PX] &= ~(1 << (Y % 8U));
    }
    else
    {
        self->config.pixel_buffer[X + (Y / 8U) * SSD1306_DISPLAY_WIDTH_PX] |= 1 << (Y % 8U);
    }
    return;
}

int ssd1306_display_update_full_screen (ssd1306_display_t * const self, std_error_t * const error)
{
    self->config.pixel_buffer[0] = SET_DISPLAY_START_LINE_COMMAND;

    if (self->config.write_i2c_dma_callback != NULL)
    {
        return self->config.write_i2c_dma_callback(self->config.device_address, self->config.pixel_buffer, SSD1306_DISPLAY_PIXEL_BUFFER_SIZE, error);
    }
    return self->config.write_i2c_callback(self->config.device_address, self->config.pixel_buffer, SSD1306_DISPLAY_PIXEL_BUFFER_SIZE, self->config.i2c_timeout_ms, error);
}


// Font data
#define ________ 0x0
#define _______X 0x1
#define ______X_ 0x2
#define ______XX 0x3
#define _____X__ 0x4
#define _____X_X 0x5
#define _____XX_ 0x6
#define _____XXX 0x7
#define ____X___ 0x8
#define ____X__X 0x9
#define ____X_X_ 0xa
#define ____X_XX 0xb
#define ____XX__ 0xc
#define ____XX_X 0xd
#define ____XXX_ 0xe
#define ____XXXX 0xf
#define ___X____ 0x10
#define ___X___X 0x11
#define ___X__X_ 0x12
#define ___X__XX 0x13
#define ___X_X__ 0x14
#define ___X_X_X 0x15
#define ___X_XX_ 0x16
#define ___X_XXX 0x17
#define ___XX___ 0x18
#define ___XX__X 0x19
#define ___XX_X_ 0x1a
#define ___XX_XX 0x1b
#define ___XXX__ 0x1c
#define ___XXX_X 0x1d
#define ___XXXX_ 0x1e
#define ___XXXXX 0x1f
#define __X_____ 0x20
#define __X____X 0x21
#define __X___X_ 0x22
#define __X___XX 0x23
#define __X__X__ 0x24
#define __X__X_X 0x25
#define __X__XX_ 0x26
#define __X__XXX 0x27
#define __X_X___ 0x28
#define __X_X__X 0x29
#define __X_X_X_ 0x2a
#define __X_X_XX 0x2b
#define __X_XX__ 0x2c
#define __X_XX_X 0x2d
#define __X_XXX_ 0x2e
#define __X_XXXX 0x2f
#define __XX____ 0x30
#define __XX___X 0x31
#define __XX__X_ 0x32
#define __XX__XX 0x33
#define __XX_X__ 0x34
#define __XX_X_X 0x35
#define __XX_XX_ 0x36
#define __XX_XXX 0x37
#define __XXX___ 0x38
#define __XXX__X 0x39
#define __XXX_X_ 0x3a
#define __XXX_XX 0x3b
#define __XXXX__ 0x3c
#define __XXXX_X 0x3d
#define __XXXXX_ 0x3e
#define __XXXXXX 0x3f
#define _X______ 0x40
#define _X_____X 0x41
#define _X____X_ 0x42
#define _X____XX 0x43
#define _X___X__ 0x44
#define _X___X_X 0x45
#define _X___XX_ 0x46
#define _X___XXX 0x47
#define _X__X___ 0x48
#define _X__X__X 0x49
#define _X__X_X_ 0x4a
#define _X__X_XX 0x4b
#define _X__XX__ 0x4c
#define _X__XX_X 0x4d
#define _X__XXX_ 0x4e
#define _X__XXXX 0x4f
#define _X_X____ 0x50
#define _X_X___X 0x51
#define _X_X__X_ 0x52
#define _X_X__XX 0x53
#define _X_X_X__ 0x54
#define _X_X_X_X 0x55
#define _X_X_XX_ 0x56
#define _X_X_XXX 0x57
#define _X_XX___ 0x58
#define _X_XX__X 0x59
#define _X_XX_X_ 0x5a
#define _X_XX_XX 0x5b
#define _X_XXX__ 0x5c
#define _X_XXX_X 0x5d
#define _X_XXXX_ 0x5e
#define _X_XXXXX 0x5f
#define _XX_____ 0x60
#define _XX____X 0x61
#define _XX___X_ 0x62
#define _XX___XX 0x63
#define _XX__X__ 0x64
#define _XX__X_X 0x65
#define _XX__XX_ 0x66
#define _XX__XXX 0x67
#define _XX_X___ 0x68
#define _XX_X__X 0x69
#define _XX_X_X_ 0x6a
#define _XX_X_XX 0x6b
#define _XX_XX__ 0x6c
#define _XX_XX_X 0x6d
#define _XX_XXX_ 0x6e
#define _XX_XXXX 0x6f
#define _XXX____ 0x70
#define _XXX___X 0x71
#define _XXX__X_ 0x72
#define _XXX__XX 0x73
#define _XXX_X__ 0x74
#define _XXX_X_X 0x75
#define _XXX_XX_ 0x76
#define _XXX_XXX 0x77
#define _XXXX___ 0x78
#define _XXXX__X 0x79
#define _XXXX_X_ 0x7a
#define _XXXX_XX 0x7b
#define _XXXXX__ 0x7c
#define _XXXXX_X 0x7d
#define _XXXXXX_ 0x7e
#define _XXXXXXX 0x7f
#define X_______ 0x80
#define X______X 0x81
#define X_____X_ 0x82
#define X_____XX 0x83
#define X____X__ 0x84
#define X____X_X 0x85
#define X____XX_ 0x86
#define X____XXX 0x87
#define X___X___ 0x88
#define X___X__X 0x89
#define X___X_X_ 0x8a
#define X___X_XX 0x8b
#define X___XX__ 0x8c
#define X___XX_X 0x8d
#define X___XXX_ 0x8e
#define X___XXXX 0x8f
#define X__X____ 0x90
#define X__X___X 0x91
#define X__X__X_ 0x92
#define X__X__XX 0x93
#define X__X_X__ 0x94
#define X__X_X_X 0x95
#define X__X_XX_ 0x96
#define X__X_XXX 0x97
#define X__XX___ 0x98
#define X__XX__X 0x99
#define X__XX_X_ 0x9a
#define X__XX_XX 0x9b
#define X__XXX__ 0x9c
#define X__XXX_X 0x9d
#define X__XXXX_ 0x9e
#define X__XXXXX 0x9f
#define X_X_____ 0xa0
#define X_X____X 0xa1
#define X_X___X_ 0xa2
#define X_X___XX 0xa3
#define X_X__X__ 0xa4
#define X_X__X_X 0xa5
#define X_X__XX_ 0xa6
#define X_X__XXX 0xa7
#define X_X_X___ 0xa8
#define X_X_X__X 0xa9
#define X_X_X_X_ 0xaa
#define X_X_X_XX 0xab
#define X_X_XX__ 0xac
#define X_X_XX_X 0xad
#define X_X_XXX_ 0xae
#define X_X_XXXX 0xaf
#define X_XX____ 0xb0
#define X_XX___X 0xb1
#define X_XX__X_ 0xb2
#define X_XX__XX 0xb3
#define X_XX_X__ 0xb4
#define X_XX_X_X 0xb5
#define X_XX_XX_ 0xb6
#define X_XX_XXX 0xb7
#define X_XXX___ 0xb8
#define X_XXX__X 0xb9
#define X_XXX_X_ 0xba
#define X_XXX_XX 0xbb
#define X_XXXX__ 0xbc
#define X_XXXX_X 0xbd
#define X_XXXXX_ 0xbe
#define X_XXXXXX 0xbf
#define XX______ 0xc0
#define XX_____X 0xc1
#define XX____X_ 0xc2
#define XX____XX 0xc3
#define XX___X__ 0xc4
#define XX___X_X 0xc5
#define XX___XX_ 0xc6
#define XX___XXX 0xc7
#define XX__X___ 0xc8
#define XX__X__X 0xc9
#define XX__X_X_ 0xca
#define XX__X_XX 0xcb
#define XX__XX__ 0xcc
#define XX__XX_X 0xcd
#define XX__XXX_ 0xce
#define XX__XXXX 0xcf
#define XX_X____ 0xd0
#define XX_X___X 0xd1
#define XX_X__X_ 0xd2
#define XX_X__XX 0xd3
#define XX_X_X__ 0xd4
#define XX_X_X_X 0xd5
#define XX_X_XX_ 0xd6
#define XX_X_XXX 0xd7
#define XX_XX___ 0xd8
#define XX_XX__X 0xd9
#define XX_XX_X_ 0xda
#define XX_XX_XX 0xdb
#define XX_XXX__ 0xdc
#define XX_XXX_X 0xdd
#define XX_XXXX_ 0xde
#define XX_XXXXX 0xdf
#define XXX_____ 0xe0
#define XXX____X 0xe1
#define XXX___X_ 0xe2
#define XXX___XX 0xe3
#define XXX__X__ 0xe4
#define XXX__X_X 0xe5
#define XXX__XX_ 0xe6
#define XXX__XXX 0xe7
#define XXX_X___ 0xe8
#define XXX_X__X 0xe9
#define XXX_X_X_ 0xea
#define XXX_X_XX 0xeb
#define XXX_XX__ 0xec
#define XXX_XX_X 0xed
#define XXX_XXX_ 0xee
#define XXX_XXXX 0xef
#define XXXX____ 0xf0
#define XXXX___X 0xf1
#define XXXX__X_ 0xf2
#define XXXX__XX 0xf3
#define XXXX_X__ 0xf4
#define XXXX_X_X 0xf5
#define XXXX_XX_ 0xf6
#define XXXX_XXX 0xf7
#define XXXXX___ 0xf8
#define XXXXX__X 0xf9
#define XXXXX_X_ 0xfa
#define XXXXX_XX 0xfb
#define XXXXXX__ 0xfc
#define XXXXXX_X 0xfd
#define XXXXXXX_ 0xfe
#define XXXXXXXX 0xff

#define f10x16_FLOAT_WIDTH 10
#define f10x16_FLOAT_HEIGHT 16
#define f10x16f_NOFCHARS 256

const uint8_t font10x16_table[f10x16f_NOFCHARS][32U + 2U] =
    {
        // 0x00
        {
            2,
            f10x16_FLOAT_HEIGHT,
            ________, ________,
            ________, ________,
            ________, ________,
            ________, ________,
            ________, ________,
            ________, ________,
            ________, ________,
            ________, ________,
            ________, ________,
            ________, ________,
            ________, ________,
            ________, ________,
            ________, ________,
            ________, ________,
            ________, ________,
            ________, ________}
        // 0x01
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x02
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x03
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x04
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x05
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x06
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x07
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x08
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x09
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x0A
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x0B
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x0C
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x0D
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x0E
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x0F
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x10
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x11
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x12
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x13
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x14
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x15
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x16
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x17
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x18
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x19
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x1A
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x1B
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x1C
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x1D
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x1E
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x1F
        ,
        {2,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x20
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x21
        ,
        {3,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         ________, ________}
        // 0x22
        ,
        {7,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX__XX__, ________,
         XX__XX__, ________,
         XX__XX__, ________,
         XX__XX__, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x23
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         __XX_XX_, ________,
         __XX_XX_, ________,
         __XX_XX_, ________,
         XXXXXXX_, ________,
         XXXXXXX_, ________,
         _XX_XX__, ________,
         _XX_XX__, ________,
         XXXXXXX_, ________,
         XXXXXXX_, ________,
         XX_XX___, ________,
         XX_XX___, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x24
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ___X____, ________,
         __XXX___, ________,
         _XXXXX__, ________,
         XX_X_XX_, ________,
         XX_X____, ________,
         XXXX____, ________,
         _XXXX___, ________,
         __XXXX__, ________,
         ___XXXX_, ________,
         XX_X_XX_, ________,
         XX_X_XX_, ________,
         _XXXXX__, ________,
         __XXX___, ________,
         ___X____, ________,
         ________, ________}
        // 0x25
        ,
        {16,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         _XXXX___, __XX____,
         XX__XX__, _XX_____,
         XX__XX__, _XX_____,
         XX__XX__, XX______,
         XX__XX_X, X_______,
         _XXXX__X, X_______,
         ______XX, __XXXX__,
         ______XX, _XX__XX_,
         _____XX_, _XX__XX_,
         _____XX_, _XX__XX_,
         ____XX__, _XX__XX_,
         ___XX___, __XXXX__,
         ________, ________,
         ________, ________}
        // 0x26
        ,
        {12,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         __XXXXX_, ________,
         _XXXXXXX, ________,
         _XX___XX, ________,
         _XX___XX, ________,
         __XXXXX_, ________,
         __XXXX__, ________,
         _XX_XX__, X_______,
         XX__XXX_, XX______,
         XX___XXX, X_______,
         XX____XX, XX______,
         _XXXXXXX, XXX_____,
         __XXXX__, _X______,
         ________, ________,
         ________, ________}
        // 0x27
        ,
        {3,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x28
        ,
        {5,
         f10x16_FLOAT_HEIGHT,
         __XX____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         __XX____, ________,
         ________, ________}
        // 0x29
        ,
        {5,
         f10x16_FLOAT_HEIGHT,
         XX______, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         XX______, ________,
         ________, ________}
        // 0x2A
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         _X_X_X__, ________,
         __XXX___, ________,
         XXXXXXX_, ________,
         __XXX___, ________,
         _X_X_X__, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x2B
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ___XX___, ________,
         ___XX___, ________,
         ___XX___, ________,
         XXXXXXXX, ________,
         XXXXXXXX, ________,
         ___XX___, ________,
         ___XX___, ________,
         ___XX___, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x2C
        ,
        {3,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         _X______, ________,
         _X______, ________,
         X_______, ________}
        // 0x2D
        ,
        {6,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XXXXX___, ________,
         XXXXX___, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x2E
        ,
        {3,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x2F
        ,
        {5,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x30
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         __XXXX__, ________,
         _XXXXXX_, ________,
         XXX__XXX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XXX__XXX, ________,
         _XXXXXX_, ________,
         __XXXX__, ________,
         ________, ________,
         ________, ________}
        // 0x31
        ,
        {6,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ___XX___, ________,
         __XXX___, ________,
         _XXXX___, ________,
         XX_XX___, ________,
         X__XX___, ________,
         ___XX___, ________,
         ___XX___, ________,
         ___XX___, ________,
         ___XX___, ________,
         ___XX___, ________,
         ___XX___, ________,
         ___XX___, ________,
         ________, ________,
         ________, ________}
        // 0x32
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         __XXXX__, ________,
         _XXXXXX_, ________,
         XXX___XX, ________,
         XX____XX, ________,
         ______XX, ________,
         _____XX_, ________,
         ____XXX_, ________,
         ___XXX__, ________,
         __XXX___, ________,
         _XX_____, ________,
         XXXXXXXX, ________,
         XXXXXXXX, ________,
         ________, ________,
         ________, ________}
        // 0x33
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         __XXXXX_, ________,
         _XXXXXXX, ________,
         XX____XX, ________,
         ______XX, ________,
         ___XXXX_, ________,
         ___XXXX_, ________,
         _____XXX, ________,
         ______XX, ________,
         XX____XX, ________,
         XXX__XXX, ________,
         _XXXXXX_, ________,
         __XXXX__, ________,
         ________, ________,
         ________, ________}
        // 0x34
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         _____XX_, ________,
         ____XXX_, ________,
         ____XXX_, ________,
         ___XXXX_, ________,
         __XX_XX_, ________,
         __XX_XX_, ________,
         _XX__XX_, ________,
         XX___XX_, ________,
         XXXXXXXX, ________,
         XXXXXXXX, ________,
         _____XX_, ________,
         _____XX_, ________,
         ________, ________,
         ________, ________}
        // 0x35
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         _XXXXXX_, ________,
         _XXXXXX_, ________,
         _XX_____, ________,
         XXX_____, ________,
         XXXXXX__, ________,
         XXXXXXX_, ________,
         XX___XXX, ________,
         ______XX, ________,
         XX____XX, ________,
         XXX__XXX, ________,
         _XXXXXX_, ________,
         __XXXX__, ________,
         ________, ________,
         ________, ________}
        // 0x36
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         __XXXXX_, ________,
         _XXXXXXX, ________,
         _XX___XX, ________,
         XX______, ________,
         XX_XXX__, ________,
         XXXXXXX_, ________,
         XXX__XXX, ________,
         XX____XX, ________,
         XX____XX, ________,
         _XX___XX, ________,
         _XXXXXX_, ________,
         __XXXX__, ________,
         ________, ________,
         ________, ________}
        // 0x37
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXXXXXXX, ________,
         XXXXXXXX, ________,
         _____XX_, ________,
         ____XX__, ________,
         ____XX__, ________,
         ___XX___, ________,
         ___XX___, ________,
         ___XX___, ________,
         __XXX___, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         ________, ________,
         ________, ________}
        // 0x38
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         __XXXX__, ________,
         _XXXXXX_, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         _XXXXXX_, ________,
         _XXXXXX_, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         _XXXXXX_, ________,
         __XXXX__, ________,
         ________, ________,
         ________, ________}
        // 0x39
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         __XXXX__, ________,
         _XXXXXX_, ________,
         XX___XX_, ________,
         XX____XX, ________,
         XX____XX, ________,
         XXX__XXX, ________,
         _XXXXXXX, ________,
         __XXX_XX, ________,
         ______XX, ________,
         XX___XX_, ________,
         XXXXXXX_, ________,
         _XXXXX__, ________,
         ________, ________,
         ________, ________}
        // 0x3A
        ,
        {3,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x3B
        ,
        {3,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         _X______, ________,
         _X______, ________,
         X_______, ________,
         ________, ________}
        // 0x3C
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         _______X, ________,
         _____XXX, ________,
         ___XXXX_, ________,
         _XXXX___, ________,
         XXX_____, ________,
         _XXXX___, ________,
         ___XXXX_, ________,
         _____XXX, ________,
         _______X, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x3D
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XXXXXXX_, ________,
         XXXXXXX_, ________,
         ________, ________,
         ________, ________,
         XXXXXXX_, ________,
         XXXXXXX_, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x3E
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         X_______, ________,
         XXX_____, ________,
         _XXXX___, ________,
         ___XXXX_, ________,
         _____XXX, ________,
         ___XXXX_, ________,
         _XXXX___, ________,
         XXX_____, ________,
         X_______, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x3F
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         __XXXX__, ________,
         _XXXXXX_, ________,
         XXX___XX, ________,
         XX____XX, ________,
         _____XXX, ________,
         ____XXX_, ________,
         ___XXX__, ________,
         ___XX___, ________,
         ___XX___, ________,
         ________, ________,
         ___XX___, ________,
         ___XX___, ________,
         ________, ________,
         ________, ________}
        // 0x40
        ,
        {16,
         f10x16_FLOAT_HEIGHT,
         _____XXX, XXX_____,
         ___XXXXX, XXXXX___,
         __XXX___, ___XXX__,
         _XXX__XX, X_XXXX__,
         _XX_XXXX, XXXX_XX_,
         XXX_XX__, _XXX_XX_,
         XX_XX___, _XX__XX_,
         XX_XX___, _XX__XX_,
         XX_XX___, _XX__XX_,
         XX_XX___, XXX_XX__,
         XX_XXXXX, XXXXX___,
         _XX_XXXX, _XXX____,
         _XXX____, _____XX_,
         __XXX___, ___XXX__,
         ___XXXXX, XXXXX___,
         _____XXX, XXX_____}
        // 0x41
        ,
        {12,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ____XXX_, ________,
         ____XXX_, ________,
         ___XX_XX, ________,
         ___XX_XX, ________,
         ___XX_XX, ________,
         __XX___X, X_______,
         __XX___X, X_______,
         __XXXXXX, X_______,
         _XXXXXXX, XX______,
         _XX_____, XX______,
         _XX_____, XX______,
         XX______, _XX_____,
         ________, ________,
         ________, ________}
        // 0x42
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXXXXXXX, ________,
         XXXXXXXX, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XXXXXXXX, ________,
         XXXXXXXX, X_______,
         XX_____X, XX______,
         XX______, XX______,
         XX______, XX______,
         XXXXXXXX, X_______,
         XXXXXXXX, ________,
         ________, ________,
         ________, ________}
        // 0x43
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ___XXXXX, ________,
         _XXXXXXX, X_______,
         _XX____X, XX______,
         XX______, X_______,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, X_______,
         _XX____X, XX______,
         _XXXXXXX, X_______,
         ___XXXXX, ________,
         ________, ________,
         ________, ________}
        // 0x44
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXXXXXX_, ________,
         XXXXXXXX, X_______,
         XX_____X, X_______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX_____X, X_______,
         XXXXXXXX, X_______,
         XXXXXXX_, ________,
         ________, ________,
         ________, ________}
        // 0x45
        ,
        {10,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXXXXXXX, X_______,
         XXXXXXXX, X_______,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XXXXXXXX, X_______,
         XXXXXXXX, X_______,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XXXXXXXX, X_______,
         XXXXXXXX, X_______,
         ________, ________,
         ________, ________}
        // 0x46
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXXXXXXX, ________,
         XXXXXXXX, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XXXXXXX_, ________,
         XXXXXXX_, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         ________, ________}
        // 0x47
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ___XXXXX, ________,
         _XXXXXXX, X_______,
         _XX____X, XX______,
         XX______, X_______,
         XX______, ________,
         XX______, ________,
         XX___XXX, XX______,
         XX___XXX, XX______,
         XX______, XX______,
         _XX____X, XX______,
         _XXXXXXX, X_______,
         ___XXXXX, ________,
         ________, ________,
         ________, ________}
        // 0x48
        ,
        {10,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XXXXXXXX, X_______,
         XXXXXXXX, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         ________, ________,
         ________, ________}
        // 0x49
        ,
        {3,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         ________, ________}
        // 0x4A
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ______XX, ________,
         ______XX, ________,
         ______XX, ________,
         ______XX, ________,
         ______XX, ________,
         ______XX, ________,
         ______XX, ________,
         ______XX, ________,
         XX____XX, ________,
         XXX__XXX, ________,
         _XXXXXX_, ________,
         __XXXX__, ________,
         ________, ________,
         ________, ________}
        // 0x4B
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, XX______,
         XX_____X, X_______,
         XX____XX, ________,
         XX___XX_, ________,
         XX__XX__, ________,
         XX_XXXX_, ________,
         XXXX_XX_, ________,
         XXX___XX, ________,
         XX____XX, ________,
         XX_____X, X_______,
         XX_____X, XX______,
         XX______, XX______,
         ________, ________,
         ________, ________}
        // 0x4C
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XXXXXXXX, ________,
         XXXXXXXX, ________,
         ________, ________,
         ________, ________}
        // 0x4D
        ,
        {12,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXX_____, XXX_____,
         XXX_____, XXX_____,
         XXXX___X, XXX_____,
         XXXX___X, XXX_____,
         XX_X___X, _XX_____,
         XX_XX_XX, _XX_____,
         XX_XX_XX, _XX_____,
         XX_XX_XX, _XX_____,
         XX__XXX_, _XX_____,
         XX__XXX_, _XX_____,
         XX__XXX_, _XX_____,
         XX___X__, _XX_____,
         ________, ________,
         ________, ________}
        // 0x4E
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, XX______,
         XXX_____, XX______,
         XXXX____, XX______,
         XXXX____, XX______,
         XX_XX___, XX______,
         XX__XX__, XX______,
         XX__XX__, XX______,
         XX___XX_, XX______,
         XX____XX, XX______,
         XX____XX, XX______,
         XX_____X, XX______,
         XX______, XX______,
         ________, ________,
         ________, ________}
        // 0x4F
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ___XXXX_, ________,
         _XXXXXXX, X_______,
         _XX____X, X_______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         _XX____X, X_______,
         _XXXXXXX, X_______,
         ___XXXX_, ________,
         ________, ________,
         ________, ________}
        // 0x50
        ,
        {10,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXXXXXX_, ________,
         XXXXXXXX, ________,
         XX____XX, X_______,
         XX_____X, X_______,
         XX____XX, X_______,
         XXXXXXXX, ________,
         XXXXXXX_, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         ________, ________}
        // 0x51
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ___XXXX_, ________,
         _XXXXXXX, X_______,
         _XX____X, X_______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX__XX__, XX______,
         _XX__XXX, X_______,
         _XXXXXXX, X_______,
         ___XXX_X, X_______,
         ________, XX______,
         ________, ________}
        // 0x52
        ,
        {12,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXXXXXXX, ________,
         XXXXXXXX, X_______,
         XX_____X, XX______,
         XX______, XX______,
         XX_____X, XX______,
         XXXXXXXX, X_______,
         XXXXXXX_, ________,
         XX___XXX, ________,
         XX____XX, X_______,
         XX_____X, X_______,
         XX_____X, XX______,
         XX______, XXX_____,
         ________, ________,
         ________, ________}
        // 0x53
        ,
        {10,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         __XXXXX_, ________,
         _XXXXXXX, ________,
         XX____XX, X_______,
         XX_____X, X_______,
         XXXX____, ________,
         _XXXXXX_, ________,
         ___XXXXX, ________,
         ______XX, X_______,
         XX_____X, X_______,
         XXX___XX, X_______,
         _XXXXXXX, ________,
         __XXXXX_, ________,
         ________, ________,
         ________, ________}
        // 0x54
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXXXXXXX, XX______,
         XXXXXXXX, XX______,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ________, ________,
         ________, ________}
        // 0x55
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XXX____X, XX______,
         _XXXXXXX, X_______,
         __XXXXXX, ________,
         ________, ________,
         ________, ________}
        // 0x56
        ,
        {12,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, _XX_____,
         XX______, _XX_____,
         _XX_____, XX______,
         _XX_____, XX______,
         __XX___X, X_______,
         __XX___X, X_______,
         __XX___X, X_______,
         ___XX_XX, ________,
         ___XX_XX, ________,
         ____XXX_, ________,
         ____XXX_, ________,
         ____XXX_, ________,
         ________, ________,
         ________, ________}
        // 0x57
        ,
        {16,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX____XX, X____XX_,
         XXX___XX, X____XX_,
         _XX___XX, X___XX__,
         _XX__XX_, XX__XX__,
         _XX__XX_, XX__XX__,
         __XX_XX_, XX_XX___,
         __XX_XX_, XX_XX___,
         __XX_XX_, XX_XX___,
         ___XXX__, _XXXX___,
         ___XXX__, _XXX____,
         ___XXX__, _XXX____,
         ___XXX__, _XXX____,
         ________, ________,
         ________, ________}
        // 0x58
        ,
        {10,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX_____X, X_______,
         XXX___XX, X_______,
         _XX___XX, ________,
         __XX_XX_, ________,
         __XXXXX_, ________,
         ___XXX__, ________,
         ___XXX__, ________,
         __XXXXX_, ________,
         __XX_XX_, ________,
         _XX___XX, ________,
         XXX___XX, X_______,
         XX_____X, X_______,
         ________, ________,
         ________, ________}
        // 0x59
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, XX______,
         XXX____X, XX______,
         _XX____X, X_______,
         __XX__XX, ________,
         __XX__XX, ________,
         ___XXXX_, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ________, ________,
         ________, ________}
        // 0x5A
        ,
        {10,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         _XXXXXXX, X_______,
         _XXXXXXX, X_______,
         ______XX, ________,
         _____XX_, ________,
         ____XXX_, ________,
         ____XX__, ________,
         ___XX___, ________,
         __XXX___, ________,
         __XX____, ________,
         _XX_____, ________,
         XXXXXXXX, X_______,
         XXXXXXXX, X_______,
         ________, ________,
         ________, ________}
        // 0x5B
        ,
        {5,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         XXXX____, ________,
         XXXX____, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XXXX____, ________,
         XXXX____, ________,
         ________, ________}
        // 0x5C
        ,
        {5,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         ________, ________,
         ________, ________}
        // 0x5D
        ,
        {5,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         XXXX____, ________,
         XXXX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         XXXX____, ________,
         XXXX____, ________,
         ________, ________}
        // 0x5E
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ___XX___, ________,
         __XXXX__, ________,
         __XXXX__, ________,
         _XX__XX_, ________,
         _XX__XX_, ________,
         XX____XX, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x5F
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XXXXXXXX, ________,
         XXXXXXXX, ________,
         ________, ________}
        // 0x60
        ,
        {4,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         XX______, ________,
         _XX_____, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x61
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         _XXXXX__, ________,
         XXXXXXX_, ________,
         XX___XX_, ________,
         ___XXXX_, ________,
         _XXXXXX_, ________,
         XXX__XX_, ________,
         XX___XX_, ________,
         XXXXXXX_, ________,
         _XXXX_XX, ________,
         ________, ________,
         ________, ________}
        // 0x62
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX_XXX__, ________,
         XXXXXXX_, ________,
         XXX__XXX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XXX__XXX, ________,
         XXXXXXX_, ________,
         XX_XXX__, ________,
         ________, ________,
         ________, ________}
        // 0x63
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         __XXXX__, ________,
         _XXXXXX_, ________,
         XXX__XX_, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XXX__XX_, ________,
         _XXXXXX_, ________,
         __XXXX__, ________,
         ________, ________,
         ________, ________}
        // 0x64
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ______XX, ________,
         ______XX, ________,
         ______XX, ________,
         __XXX_XX, ________,
         _XXXXXXX, ________,
         XXX__XXX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XXX__XXX, ________,
         _XXXXXXX, ________,
         __XXX_XX, ________,
         ________, ________,
         ________, ________}
        // 0x65
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         __XXX___, ________,
         _XXXXX__, ________,
         XX___XX_, ________,
         XXXXXXX_, ________,
         XXXXXXX_, ________,
         XX______, ________,
         XXX__XX_, ________,
         _XXXXX__, ________,
         __XXX___, ________,
         ________, ________,
         ________, ________}
        // 0x66
        ,
        {7,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         __XXXX__, ________,
         _XXXXX__, ________,
         _XX_____, ________,
         XXXXX___, ________,
         XXXXX___, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         ________, ________,
         ________, ________}
        // 0x67
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         __XXX_XX, ________,
         _XXXXXXX, ________,
         XXX__XXX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XXX__XXX, ________,
         _XXXXXXX, ________,
         __XXX_XX, ________,
         XX____XX, ________,
         XXXXXXXX, ________,
         _XXXXXX_, ________}
        // 0x68
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX_XXXX_, ________,
         XXXXXXXX, ________,
         XXX___XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         ________, ________,
         ________, ________}
        // 0x69
        ,
        {3,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         ________, ________}
        // 0x6A
        ,
        {4,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         _XX_____, ________,
         _XX_____, ________,
         ________, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         XXX_____, ________,
         XX______, ________}
        // 0x6B
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX___XX_, ________,
         XX__XX__, ________,
         XX_XX___, ________,
         XXXXX___, ________,
         XXXXX___, ________,
         XXX_XX__, ________,
         XX__XX__, ________,
         XX___XX_, ________,
         XX___XX_, ________,
         ________, ________,
         ________, ________}
        // 0x6C
        ,
        {3,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         ________, ________}
        // 0x6D
        ,
        {13,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX_XXX__, XXX_____,
         XXXXXXXX, XXXX____,
         XXX__XXX, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         ________, ________,
         ________, ________}
        // 0x6E
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX_XXXX_, ________,
         XXXXXXXX, ________,
         XXX___XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         ________, ________,
         ________, ________}
        // 0x6F
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         __XXXX__, ________,
         _XXXXXX_, ________,
         XXX__XXX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XXX__XXX, ________,
         _XXXXXX_, ________,
         __XXXX__, ________,
         ________, ________,
         ________, ________}
        // 0x70
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX_XXXX_, ________,
         XXXXXXXX, ________,
         XXX___XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XXX__XXX, ________,
         XXXXXXX_, ________,
         XX_XXX__, ________,
         XX______, ________,
         XX______, ________}
        // 0x71
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         __XXX_XX, ________,
         _XXXXXXX, ________,
         XXX__XXX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XXX__XXX, ________,
         _XXXXXXX, ________,
         __XXX_XX, ________,
         ______XX, ________,
         ______XX, ________,
         ______XX, ________}
        // 0x72
        ,
        {6,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX_XX___, ________,
         XXXXX___, ________,
         XXX_____, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         ________, ________}
        // 0x73
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         _XXXXX__, ________,
         XXXXXXX_, ________,
         XX___XX_, ________,
         XXXX____, ________,
         _XXXXX__, ________,
         ___XXXX_, ________,
         XX___XX_, ________,
         XXXXXXX_, ________,
         _XXXXX__, ________,
         ________, ________,
         ________, ________}
        // 0x74
        ,
        {6,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         __X_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         XXXXX___, ________,
         XXXXX___, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XX_____, ________,
         _XXXX___, ________,
         __XXX___, ________,
         ________, ________,
         ________, ________}
        // 0x75
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX___XXX, ________,
         XXXXXXXX, ________,
         _XXXX_XX, ________,
         ________, ________,
         ________, ________}
        // 0x76
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX___XX_, ________,
         XX___XX_, ________,
         XX___XX_, ________,
         _XX_XX__, ________,
         _XX_XX__, ________,
         _XX_XX__, ________,
         __XXX___, ________,
         __XXX___, ________,
         __XXX___, ________,
         ________, ________,
         ________, ________}
        // 0x77
        ,
        {14,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX___XXX, ___XX___,
         XX___XXX, ___XX___,
         _XX__XXX, __XX____,
         _XX_XX_X, X_XX____,
         _XX_XX_X, X_XX____,
         _XX_XX_X, X_XX____,
         __XXX___, XXX_____,
         __XXX___, XXX_____,
         __XXX___, XXX_____,
         ________, ________,
         ________, ________}
        // 0x78
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX___XX_, ________,
         XXX_XXX_, ________,
         _XX_XX__, ________,
         __XXX___, ________,
         __XXX___, ________,
         __XXX___, ________,
         _XX_XX__, ________,
         XXX_XXX_, ________,
         XX___XX_, ________,
         ________, ________,
         ________, ________}
        // 0x79
        ,
        {10,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX_____X, X_______,
         _XX___XX, ________,
         _XX___XX, ________,
         __XX_XX_, ________,
         __XX_XX_, ________,
         __XXXXX_, ________,
         ___XXX__, ________,
         ___XXX__, ________,
         ___XX___, ________,
         _XXXX___, ________,
         _XXX____, ________}
        // 0x7A
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XXXXXXX_, ________,
         XXXXXXX_, ________,
         ____XX__, ________,
         ___XXX__, ________,
         __XXX___, ________,
         _XXX____, ________,
         _XX_____, ________,
         XXXXXXX_, ________,
         XXXXXXX_, ________,
         ________, ________,
         ________, ________}
        // 0x7B
        ,
        {7,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ___XXX__, ________,
         __XXXX__, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         XXX_____, ________,
         XXX_____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XXXX__, ________,
         ___XXX__, ________,
         ________, ________}
        // 0x7C
        ,
        {3,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         ________, ________}
        // 0x7D
        ,
        {7,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         XXX_____, ________,
         XXXX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         ___XXX__, ________,
         ___XXX__, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         XXXX____, ________,
         XXX_____, ________,
         ________, ________}
        // 0x7E
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         _XXX___X, ________,
         XXXXXXXX, ________,
         X___XXX_, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x7F
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x80
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x81
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x82
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x83
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x84
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x85
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x86
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x87
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x88
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x89
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x8A
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x8B
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x8C
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x8D
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x8E
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x8F
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x90
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x91
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x92
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x93
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x94
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x95
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x96
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x97
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x98
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x99
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x9A
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x9B
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x9C
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x9D
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x9E
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0x9F
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xA0
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xA1
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xA2
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xA3
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xA4
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xA5
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xA6
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xA7
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xA8
        ,
        {10,
         f10x16_FLOAT_HEIGHT,
         __XX_XX_, ________,
         ________, ________,
         XXXXXXXX, X_______,
         XXXXXXXX, X_______,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XXXXXXXX, X_______,
         XXXXXXXX, X_______,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XXXXXXXX, X_______,
         XXXXXXXX, X_______,
         ________, ________,
         ________, ________}
        // 0xA9
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xAA
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xAB
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xAC
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xAD
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xAE
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xAF
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xB0
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xB1
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xB2
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xB3
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xB4
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xB5
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xB6
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xB7
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xB8
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         _XX_XX__, ________,
         _XX_XX__, ________,
         ________, ________,
         __XXX___, ________,
         _XXXXX__, ________,
         XX___XX_, ________,
         XXXXXXX_, ________,
         XXXXXXX_, ________,
         XX______, ________,
         XXX__XX_, ________,
         _XXXXX__, ________,
         __XXX___, ________,
         ________, ________,
         ________, ________}
        // 0xB9
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xBA
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xBB
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xBC
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xBD
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xBE
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xBF
        ,
        {1,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________}
        // 0xC0
        ,
        {12,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ____XXX_, ________,
         ____XXX_, ________,
         ___XX_XX, ________,
         ___XX_XX, ________,
         ___XX_XX, ________,
         __XX___X, X_______,
         __XX___X, X_______,
         __XXXXXX, X_______,
         _XXXXXXX, XX______,
         _XX_____, XX______,
         _XX_____, XX______,
         XX______, _XX_____,
         ________, ________,
         ________, ________}
        // 0xC1
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXXXXXXX, X_______,
         XXXXXXXX, X_______,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XXXXXXXX, ________,
         XXXXXXXX, X_______,
         XX_____X, XX______,
         XX______, XX______,
         XX______, XX______,
         XXXXXXXX, X_______,
         XXXXXXXX, ________,
         ________, ________,
         ________, ________}
        // 0xC2
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXXXXXXX, ________,
         XXXXXXXX, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XXXXXXXX, ________,
         XXXXXXXX, X_______,
         XX_____X, XX______,
         XX______, XX______,
         XX______, XX______,
         XXXXXXXX, X_______,
         XXXXXXXX, ________,
         ________, ________,
         ________, ________}
        // 0xC3
        ,
        {10,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXXXXXXX, X_______,
         XXXXXXXX, X_______,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         ________, ________}
        // 0xC4
        ,
        {13,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ____XXXX, XX______,
         ___XXXXX, XX______,
         ___XX___, XX______,
         ___XX___, XX______,
         ___XX___, XX______,
         __XX____, XX______,
         __XX____, XX______,
         __XX____, XX______,
         __XX____, XX______,
         _XX_____, XX______,
         XXXXXXXX, XXXX____,
         XXXXXXXX, XXXX____,
         XX______, __XX____,
         XX______, __XX____}
        // 0xC5
        ,
        {10,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXXXXXXX, X_______,
         XXXXXXXX, X_______,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XXXXXXXX, X_______,
         XXXXXXXX, X_______,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XXXXXXXX, X_______,
         XXXXXXXX, X_______,
         ________, ________,
         ________, ________}
        // 0xC6
        ,
        {13,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX___XX_, __XX____,
         _XX__XX_, _XX_____,
         _XX__XX_, _XX_____,
         _XX__XX_, _XX_____,
         __XX_XX_, XX______,
         ____XXXX, ________,
         __XX_XX_, XX______,
         __XX_XX_, XX______,
         _XX__XX_, _XX_____,
         _XX__XX_, _XX_____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         ________, ________,
         ________, ________}
        // 0xC7
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         __XXXXX_, ________,
         _XXXXXXX, ________,
         XX____XX, ________,
         ______XX, ________,
         ___XXXX_, ________,
         ___XXXX_, ________,
         _____XXX, ________,
         ______XX, ________,
         XX____XX, ________,
         XXX__XXX, ________,
         _XXXXXX_, ________,
         __XXXX__, ________,
         ________, ________,
         ________, ________}
        // 0xC8
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, XX______,
         XX_____X, XX______,
         XX____XX, XX______,
         XX____XX, XX______,
         XX___XX_, XX______,
         XX__XX__, XX______,
         XX__XX__, XX______,
         XX_XX___, XX______,
         XXXX____, XX______,
         XXXX____, XX______,
         XXX_____, XX______,
         XX______, XX______,
         ________, ________,
         ________, ________}
        // 0xC9
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ___XXXX_, ________,
         ____XX__, ________,
         XX______, XX______,
         XX_____X, XX______,
         XX____XX, XX______,
         XX____XX, XX______,
         XX___XX_, XX______,
         XX__XX__, XX______,
         XX__XX__, XX______,
         XX_XX___, XX______,
         XXXX____, XX______,
         XXXX____, XX______,
         XXX_____, XX______,
         XX______, XX______,
         ________, ________,
         ________, ________}
        // 0xCA
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, XX______,
         XX_____X, X_______,
         XX____XX, ________,
         XX___XX_, ________,
         XX__XX__, ________,
         XX_XXXX_, ________,
         XXXX_XX_, ________,
         XXX___XX, ________,
         XX____XX, ________,
         XX_____X, X_______,
         XX_____X, XX______,
         XX______, XX______,
         ________, ________,
         ________, ________}
        // 0xCB
        ,
        {10,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ___XXXXX, X_______,
         __XXXXXX, X_______,
         __XX___X, X_______,
         _XX____X, X_______,
         _XX____X, X_______,
         _XX____X, X_______,
         _XX____X, X_______,
         _XX____X, X_______,
         _XX____X, X_______,
         _XX____X, X_______,
         XXX____X, X_______,
         XX_____X, X_______,
         ________, ________,
         ________, ________}
        // 0xCC
        ,
        {12,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXX_____, XXX_____,
         XXX_____, XXX_____,
         XXXX___X, XXX_____,
         XXXX___X, XXX_____,
         XX_X___X, _XX_____,
         XX_XX_XX, _XX_____,
         XX_XX_XX, _XX_____,
         XX_XX_XX, _XX_____,
         XX__XXX_, _XX_____,
         XX__XXX_, _XX_____,
         XX__XXX_, _XX_____,
         XX___X__, _XX_____,
         ________, ________,
         ________, ________}
        // 0xCD
        ,
        {10,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XXXXXXXX, X_______,
         XXXXXXXX, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         ________, ________,
         ________, ________}
        // 0xCE
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ___XXXX_, ________,
         _XXXXXXX, X_______,
         _XX____X, X_______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         _XX____X, X_______,
         _XXXXXXX, X_______,
         ___XXXX_, ________,
         ________, ________,
         ________, ________}
        // 0xCF
        ,
        {10,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXXXXXXX, X_______,
         XXXXXXXX, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         ________, ________,
         ________, ________}
        // 0xD0
        ,
        {10,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXXXXXX_, ________,
         XXXXXXXX, ________,
         XX____XX, X_______,
         XX_____X, X_______,
         XX____XX, X_______,
         XXXXXXXX, ________,
         XXXXXXX_, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         ________, ________}
        // 0xD1
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ___XXXXX, ________,
         _XXXXXXX, X_______,
         _XX____X, XX______,
         XX______, X_______,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, X_______,
         _XX____X, XX______,
         _XXXXXXX, X_______,
         ___XXXXX, ________,
         ________, ________,
         ________, ________}
        // 0xD2
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXXXXXXX, XX______,
         XXXXXXXX, XX______,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ____XX__, ________,
         ________, ________,
         ________, ________}
        // 0xD3
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, XX______,
         XX______, XX______,
         _XX____X, X_______,
         _XX____X, X_______,
         __XX__XX, ________,
         __XX__XX, ________,
         ___XXXX_, ________,
         ___XXXX_, ________,
         ____XX__, ________,
         ____XX__, ________,
         _XXXX___, ________,
         _XXX____, ________,
         ________, ________,
         ________, ________}
        // 0xD4
        ,
        {15,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ______XX, ________,
         ___XXXXX, XXX_____,
         _XXXXXXX, XXXXX___,
         _XX___XX, ___XX___,
         XX____XX, ____XX__,
         XX____XX, ____XX__,
         XX____XX, ____XX__,
         XX____XX, ____XX__,
         _XX___XX, ___XX___,
         _XXXXXXX, XXXXX___,
         ___XXXXX, XXX_____,
         ______XX, ________,
         ________, ________,
         ________, ________}
        // 0xD5
        ,
        {10,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX_____X, X_______,
         XXX___XX, X_______,
         _XX___XX, ________,
         __XX_XX_, ________,
         __XXXXX_, ________,
         ___XXX__, ________,
         ___XXX__, ________,
         __XXXXX_, ________,
         __XX_XX_, ________,
         _XX___XX, ________,
         XXX___XX, X_______,
         XX_____X, X_______,
         ________, ________,
         ________, ________}
        // 0xD6
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XX_____X, X_______,
         XXXXXXXX, XX______,
         XXXXXXXX, XX______,
         ________, XX______,
         ________, XX______}
        // 0xD7
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XX______, XX______,
         XXX_____, XX______,
         _XXXXXXX, XX______,
         __XXXXXX, XX______,
         ________, XX______,
         ________, XX______,
         ________, XX______,
         ________, XX______,
         ________, ________,
         ________, ________}
        // 0xD8
        ,
        {13,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XXXXXXXX, XXXX____,
         XXXXXXXX, XXXX____,
         ________, ________,
         ________, ________}
        // 0xD9
        ,
        {14,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XXXXXXXX, XXXXX___,
         XXXXXXXX, XXXXX___,
         ________, ___XX___,
         ________, ___XX___}
        // 0xDA
        ,
        {12,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XXXX____, ________,
         XXXX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XX____, ________,
         __XXXXXX, X_______,
         __XXXXXX, XX______,
         __XX____, XXX_____,
         __XX____, _XX_____,
         __XX____, XXX_____,
         __XXXXXX, XX______,
         __XXXXXX, X_______,
         ________, ________,
         ________, ________}
        // 0xDB
        ,
        {13,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, __XX____,
         XX______, __XX____,
         XX______, __XX____,
         XX______, __XX____,
         XX______, __XX____,
         XXXXXXX_, __XX____,
         XXXXXXXX, __XX____,
         XX____XX, X_XX____,
         XX_____X, X_XX____,
         XX____XX, X_XX____,
         XXXXXXXX, __XX____,
         XXXXXXX_, __XX____,
         ________, ________,
         ________, ________}
        // 0xDC
        ,
        {10,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XXXXXXX_, ________,
         XXXXXXXX, ________,
         XX____XX, X_______,
         XX_____X, X_______,
         XX____XX, X_______,
         XXXXXXXX, ________,
         XXXXXXX_, ________,
         ________, ________,
         ________, ________}
        // 0xDD
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ___XXXX_, ________,
         _XXXXXXX, X_______,
         XXX____X, X_______,
         XX______, XX______,
         ________, XX______,
         ____XXXX, XX______,
         ____XXXX, XX______,
         ________, XX______,
         XX______, XX______,
         XXX____X, X_______,
         _XXXXXXX, X_______,
         ___XXXX_, ________,
         ________, ________,
         ________, ________}
        // 0xDE
        ,
        {14,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         XX_____X, XX______,
         XX___XXX, XXXX____,
         XX___XX_, __XX____,
         XX__XX__, ___XX___,
         XX__XX__, ___XX___,
         XXXXXX__, ___XX___,
         XXXXXX__, ___XX___,
         XX__XX__, ___XX___,
         XX__XX__, ___XX___,
         XX___XX_, __XX____,
         XX___XXX, XXXX____,
         XX_____X, XX______,
         ________, ________,
         ________, ________}
        // 0xDF
        ,
        {12,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ___XXXXX, XXX_____,
         __XXXXXX, XXX_____,
         _XXX____, _XX_____,
         _XX_____, _XX_____,
         _XXX____, _XX_____,
         __XXXXXX, XXX_____,
         ____XXXX, XXX_____,
         ___XXX__, _XX_____,
         __XXX___, _XX_____,
         __XX____, _XX_____,
         _XXX____, _XX_____,
         XXX_____, _XX_____,
         ________, ________,
         ________, ________}
        // 0xE0
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         _XXXXX__, ________,
         XXXXXXX_, ________,
         XX___XX_, ________,
         ___XXXX_, ________,
         _XXXXXX_, ________,
         XXX__XX_, ________,
         XX___XX_, ________,
         XXXXXXX_, ________,
         _XXXX_XX, ________,
         ________, ________,
         ________, ________}
        // 0xE1
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ______X_, ________,
         _XXXXXX_, ________,
         XXXXXX__, ________,
         XX______, ________,
         XXXXXX__, ________,
         XXXXXXX_, ________,
         XXX__XXX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XXX__XXX, ________,
         _XXXXXX_, ________,
         __XXXX__, ________,
         ________, ________,
         ________, ________}
        // 0xE2
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XXXXXX__, ________,
         XXXXXXX_, ________,
         XX___XX_, ________,
         XXXXXX__, ________,
         XXXXXX__, ________,
         XX___XX_, ________,
         XX___XX_, ________,
         XXXXXXX_, ________,
         XXXXXX__, ________,
         ________, ________,
         ________, ________}
        // 0xE3
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XXXXXXX_, ________,
         XXXXXXX_, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         ________, ________,
         ________, ________}
        // 0xE4
        ,
        {12,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ___XXXXX, X_______,
         __XXXXXX, X_______,
         __XX___X, X_______,
         __XX___X, X_______,
         __XX___X, X_______,
         __XX___X, X_______,
         _XX____X, X_______,
         XXXXXXXX, XXX_____,
         XXXXXXXX, XXX_____,
         XX______, _XX_____,
         XX______, _XX_____}
        // 0xE5
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         __XXX___, ________,
         _XXXXX__, ________,
         XX___XX_, ________,
         XXXXXXX_, ________,
         XXXXXXX_, ________,
         XX______, ________,
         XXX__XX_, ________,
         _XXXXX__, ________,
         __XXX___, ________,
         ________, ________,
         ________, ________}
        // 0xE6
        ,
        {13,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX___XX_, __XX____,
         _XX__XX_, _XX_____,
         __XX_XX_, XX______,
         __XX_XX_, XX______,
         ____XXXX, ________,
         __XX_XX_, XX______,
         _XX__XX_, _XX_____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         ________, ________,
         ________, ________}
        // 0xE7
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         _XXXXXX_, ________,
         XXXXXXXX, ________,
         ______XX, ________,
         ____XXX_, ________,
         ____XXXX, ________,
         ______XX, ________,
         XX____XX, ________,
         XXXXXXXX, ________,
         _XXXXXX_, ________,
         ________, ________,
         ________, ________}
        // 0xE8
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX____XX, ________,
         XX___XXX, ________,
         XX__XXXX, ________,
         XX_XXXXX, ________,
         XXXXX_XX, ________,
         XXXX__XX, ________,
         XXX___XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         ________, ________,
         ________, ________}
        // 0xE9
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         __X__X__, ________,
         __XXXX__, ________,
         ___XX___, ________,
         ________, ________,
         XX____XX, ________,
         XX___XXX, ________,
         XX__XXXX, ________,
         XX_XXXXX, ________,
         XXXXX_XX, ________,
         XXXX__XX, ________,
         XXX___XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         ________, ________,
         ________, ________}
        // 0xEA
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX___XX_, ________,
         XX___XX_, ________,
         XX__XX__, ________,
         XXXX____, ________,
         XXXX____, ________,
         XX__XX__, ________,
         XX___XX_, ________,
         XX___XX_, ________,
         XX___XX_, ________,
         ________, ________,
         ________, ________}
        // 0xEB
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         __XXXXXX, ________,
         _XXXXXXX, ________,
         _XX___XX, ________,
         _XX___XX, ________,
         _XX___XX, ________,
         _XX___XX, ________,
         _XX___XX, ________,
         XXX___XX, ________,
         XX____XX, ________,
         ________, ________,
         ________, ________}
        // 0xEC
        ,
        {12,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XXX_____, XXX_____,
         XXX_____, XXX_____,
         XXXX___X, XXX_____,
         XXXX___X, XXX_____,
         XX_XX_XX, _XX_____,
         XX_XX_XX, _XX_____,
         XX__XXX_, _XX_____,
         XX__XXX_, _XX_____,
         XX___X__, _XX_____,
         ________, ________,
         ________, ________}
        // 0xED
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XXXXXXXX, ________,
         XXXXXXXX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         ________, ________,
         ________, ________}
        // 0xEE
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         __XXXX__, ________,
         _XXXXXX_, ________,
         XXX__XXX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XXX__XXX, ________,
         _XXXXXX_, ________,
         __XXXX__, ________,
         ________, ________,
         ________, ________}
        // 0xEF
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XXXXXXXX, ________,
         XXXXXXXX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         ________, ________,
         ________, ________}
        // 0xF0
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX_XXXX_, ________,
         XXXXXXXX, ________,
         XXX___XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XXX__XXX, ________,
         XXXXXXX_, ________,
         XX_XXX__, ________,
         XX______, ________,
         XX______, ________}
        // 0xF1
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         __XXXX__, ________,
         _XXXXXX_, ________,
         XXX__XX_, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XXX__XX_, ________,
         _XXXXXX_, ________,
         __XXXX__, ________,
         ________, ________,
         ________, ________}
        // 0xF2
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XXXXXXXX, ________,
         XXXXXXXX, ________,
         ___XX___, ________,
         ___XX___, ________,
         ___XX___, ________,
         ___XX___, ________,
         ___XX___, ________,
         ___XX___, ________,
         ___XX___, ________,
         ________, ________,
         ________, ________}
        // 0xF3
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX______, XX______,
         XX______, XX______,
         _XX____X, X_______,
         _XX____X, X_______,
         __XX__XX, ________,
         __XX__XX, ________,
         ___XXXX_, ________,
         ___XXXX_, ________,
         ____XX__, ________,
         _XXXX___, ________,
         _XXX____, ________}
        // 0xF4
        ,
        {13,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         _____XX_, ________,
         _____XX_, ________,
         __XXXXXX, XX______,
         _XXXXXXX, XXX_____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         XX___XX_, __XX____,
         _XXXXXXX, XXX_____,
         __XXXXXX, XX______,
         _____XX_, ________,
         _____XX_, ________}
        // 0xF5
        ,
        {8,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX___XX_, ________,
         XXX_XXX_, ________,
         _XX_XX__, ________,
         __XXX___, ________,
         __XXX___, ________,
         __XXX___, ________,
         _XX_XX__, ________,
         XXX_XXX_, ________,
         XX___XX_, ________,
         ________, ________,
         ________, ________}
        // 0xF6
        ,
        {10,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XXXXXXXX, X_______,
         XXXXXXXX, X_______,
         _______X, X_______,
         _______X, X_______}
        // 0xF7
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XXXXXXXX, ________,
         _XXXXXXX, ________,
         ______XX, ________,
         ______XX, ________,
         ______XX, ________,
         ________, ________,
         ________, ________}
        // 0xF8
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX__XX__, XX______,
         XX__XX__, XX______,
         XX__XX__, XX______,
         XX__XX__, XX______,
         XX__XX__, XX______,
         XX__XX__, XX______,
         XX__XX__, XX______,
         XXXXXXXX, XX______,
         XXXXXXXX, XX______,
         ________, ________,
         ________, ________}
        // 0xF9
        ,
        {12,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX__XX__, XX______,
         XX__XX__, XX______,
         XX__XX__, XX______,
         XX__XX__, XX______,
         XX__XX__, XX______,
         XX__XX__, XX______,
         XX__XX__, XX______,
         XXXXXXXX, XX______,
         XXXXXXXX, XXX_____,
         ________, _XX_____,
         ________, _XX_____}
        // 0xFA
        ,
        {11,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XXXX____, ________,
         XXXX____, ________,
         __XX____, ________,
         __XXXXXX, X_______,
         __XXXXXX, XX______,
         __XX____, XX______,
         __XX____, XX______,
         __XXXXXX, XX______,
         __XXXXXX, X_______,
         ________, ________,
         ________, ________}
        // 0xFB
        ,
        {12,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX______, _XX_____,
         XX______, _XX_____,
         XX______, _XX_____,
         XXXXXXX_, _XX_____,
         XXXXXXXX, _XX_____,
         XX____XX, _XX_____,
         XX____XX, _XX_____,
         XXXXXXXX, _XX_____,
         XXXXXXX_, _XX_____,
         ________, ________,
         ________, ________}
        // 0xFC
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX______, ________,
         XX______, ________,
         XX______, ________,
         XXXXXXX_, ________,
         XXXXXXXX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XXXXXXXX, ________,
         XXXXXXX_, ________,
         ________, ________,
         ________, ________}
        // 0xFD
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         _XXXXXX_, ________,
         XXXXXXXX, ________,
         XX____XX, ________,
         ____XXXX, ________,
         ____XXXX, ________,
         ______XX, ________,
         XX____XX, ________,
         XXXXXXXX, ________,
         _XXXXXX_, ________,
         ________, ________,
         ________, ________}
        // 0xFE
        ,
        {12,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         XX___XXX, X_______,
         XX__XXXX, XX______,
         XX_XXX__, XXX_____,
         XXXXX___, _XX_____,
         XXXXX___, _XX_____,
         XX_XX___, _XX_____,
         XX_XXX__, XXX_____,
         XX__XXXX, XX______,
         XX___XXX, X_______,
         ________, ________,
         ________, ________}
        // 0xFF
        ,
        {9,
         f10x16_FLOAT_HEIGHT,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         ________, ________,
         _XXXXXXX, ________,
         XXXXXXXX, ________,
         XX____XX, ________,
         XX____XX, ________,
         XXXXXXXX, ________,
         _XXXXXXX, ________,
         __XX__XX, ________,
         _XX___XX, ________,
         XX____XX, ________,
         ________, ________,
         ________, ________}
};

const uint16_t font16x26_table[] =
{
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [ ]
0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03C0,0x03C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x0000,0x0000,0x0000,0x03E0,0x03E0,0x03E0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [!]
0x1E3C,0x1E3C,0x1E3C,0x1E3C,0x1E3C,0x1E3C,0x1E3C,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = ["]
0x01CE,0x03CE,0x03DE,0x039E,0x039C,0x079C,0x3FFF,0x7FFF,0x0738,0x0F38,0x0F78,0x0F78,0x0E78,0xFFFF,0xFFFF,0x1EF0,0x1CF0,0x1CE0,0x3CE0,0x3DE0,0x39E0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [#]
0x03FC,0x0FFE,0x1FEE,0x1EE0,0x1EE0,0x1EE0,0x1EE0,0x1FE0,0x0FE0,0x07E0,0x03F0,0x01FC,0x01FE,0x01FE,0x01FE,0x01FE,0x01FE,0x01FE,0x3DFE,0x3FFC,0x0FF0,0x01E0,0x01E0,0x0000,0x0000,0x0000, // Ascii = [$]
0x3E03,0xF707,0xE78F,0xE78E,0xE39E,0xE3BC,0xE7B8,0xE7F8,0xF7F0,0x3FE0,0x01C0,0x03FF,0x07FF,0x07F3,0x0FF3,0x1EF3,0x3CF3,0x38F3,0x78F3,0xF07F,0xE03F,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [%]
0x07E0,0x0FF8,0x0F78,0x1F78,0x1F78,0x1F78,0x0F78,0x0FF0,0x0FE0,0x1F80,0x7FC3,0xFBC3,0xF3E7,0xF1F7,0xF0F7,0xF0FF,0xF07F,0xF83E,0x7C7F,0x3FFF,0x1FEF,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [&]
0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03C0,0x01C0,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [']
0x003F,0x007C,0x01F0,0x01E0,0x03C0,0x07C0,0x0780,0x0780,0x0F80,0x0F00,0x0F00,0x0F00,0x0F00,0x0F00,0x0F00,0x0F80,0x0780,0x0780,0x07C0,0x03C0,0x01E0,0x01F0,0x007C,0x003F,0x000F,0x0000, // Ascii = [(]
0x7E00,0x1F00,0x07C0,0x03C0,0x01E0,0x01F0,0x00F0,0x00F0,0x00F8,0x0078,0x0078,0x0078,0x0078,0x0078,0x0078,0x00F8,0x00F0,0x00F0,0x01F0,0x01E0,0x03C0,0x07C0,0x1F00,0x7E00,0x7800,0x0000, // Ascii = [)]
0x03E0,0x03C0,0x01C0,0x39CE,0x3FFF,0x3F7F,0x0320,0x0370,0x07F8,0x0F78,0x1F3C,0x0638,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [*]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0xFFFF,0xFFFF,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [+]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x03E0,0x03E0,0x03E0,0x03E0,0x01E0,0x01E0,0x01E0,0x01C0,0x0380, // Ascii = [,]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x3FFE,0x3FFE,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [-]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x03E0,0x03E0,0x03E0,0x03E0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [.]
0x000F,0x000F,0x001E,0x001E,0x003C,0x003C,0x0078,0x0078,0x00F0,0x00F0,0x01E0,0x01E0,0x03C0,0x03C0,0x0780,0x0780,0x0F00,0x0F00,0x1E00,0x1E00,0x3C00,0x3C00,0x7800,0x7800,0xF000,0x0000, // Ascii = [/]
0x07F0,0x0FF8,0x1F7C,0x3E3E,0x3C1E,0x7C1F,0x7C1F,0x780F,0x780F,0x780F,0x780F,0x780F,0x780F,0x780F,0x7C1F,0x7C1F,0x3C1E,0x3E3E,0x1F7C,0x0FF8,0x07F0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [0]
0x00F0,0x07F0,0x3FF0,0x3FF0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x3FFF,0x3FFF,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [1]
0x0FE0,0x3FF8,0x3C7C,0x003C,0x003E,0x003E,0x003E,0x003C,0x003C,0x007C,0x00F8,0x01F0,0x03E0,0x07C0,0x0780,0x0F00,0x1E00,0x3E00,0x3C00,0x3FFE,0x3FFE,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [2]
0x0FF0,0x1FF8,0x1C7C,0x003E,0x003E,0x003E,0x003C,0x003C,0x00F8,0x0FF0,0x0FF8,0x007C,0x003E,0x001E,0x001E,0x001E,0x001E,0x003E,0x1C7C,0x1FF8,0x1FE0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [3]
0x0078,0x00F8,0x00F8,0x01F8,0x03F8,0x07F8,0x07F8,0x0F78,0x1E78,0x1E78,0x3C78,0x7878,0x7878,0xFFFF,0xFFFF,0x0078,0x0078,0x0078,0x0078,0x0078,0x0078,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [4]
0x1FFC,0x1FFC,0x1FFC,0x1E00,0x1E00,0x1E00,0x1E00,0x1E00,0x1FE0,0x1FF8,0x00FC,0x007C,0x003E,0x003E,0x001E,0x003E,0x003E,0x003C,0x1C7C,0x1FF8,0x1FE0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [5]
0x01FC,0x07FE,0x0F8E,0x1F00,0x1E00,0x3E00,0x3C00,0x3C00,0x3DF8,0x3FFC,0x7F3E,0x7E1F,0x3C0F,0x3C0F,0x3C0F,0x3C0F,0x3E0F,0x1E1F,0x1F3E,0x0FFC,0x03F0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [6]
0x3FFF,0x3FFF,0x3FFF,0x000F,0x001E,0x001E,0x003C,0x0038,0x0078,0x00F0,0x00F0,0x01E0,0x01E0,0x03C0,0x03C0,0x0780,0x0F80,0x0F80,0x0F00,0x1F00,0x1F00,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [7]
0x07F8,0x0FFC,0x1F3E,0x1E1E,0x3E1E,0x3E1E,0x1E1E,0x1F3C,0x0FF8,0x07F0,0x0FF8,0x1EFC,0x3E3E,0x3C1F,0x7C1F,0x7C0F,0x7C0F,0x3C1F,0x3F3E,0x1FFC,0x07F0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [8]
0x07F0,0x0FF8,0x1E7C,0x3C3E,0x3C1E,0x7C1F,0x7C1F,0x7C1F,0x7C1F,0x3C1F,0x3E3F,0x1FFF,0x07EF,0x001F,0x001E,0x001E,0x003E,0x003C,0x38F8,0x3FF0,0x1FE0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [9]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x03E0,0x03E0,0x03E0,0x03E0,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x03E0,0x03E0,0x03E0,0x03E0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [:]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x03E0,0x03E0,0x03E0,0x03E0,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x03E0,0x03E0,0x03E0,0x03E0,0x01E0,0x01E0,0x01E0,0x03C0,0x0380, // Ascii = [;]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0003,0x000F,0x003F,0x00FC,0x03F0,0x0FC0,0x3F00,0xFE00,0x3F00,0x0FC0,0x03F0,0x00FC,0x003F,0x000F,0x0003,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [<]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0xFFFF,0xFFFF,0x0000,0x0000,0x0000,0xFFFF,0xFFFF,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [=]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0xE000,0xF800,0x7E00,0x1F80,0x07E0,0x01F8,0x007E,0x001F,0x007E,0x01F8,0x07E0,0x1F80,0x7E00,0xF800,0xE000,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [>]
0x1FF0,0x3FFC,0x383E,0x381F,0x381F,0x001E,0x001E,0x003C,0x0078,0x00F0,0x01E0,0x03C0,0x03C0,0x07C0,0x07C0,0x0000,0x0000,0x0000,0x07C0,0x07C0,0x07C0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [?]
0x03F8,0x0FFE,0x1F1E,0x3E0F,0x3C7F,0x78FF,0x79EF,0x73C7,0xF3C7,0xF38F,0xF38F,0xF38F,0xF39F,0xF39F,0x73FF,0x7BFF,0x79F7,0x3C00,0x1F1C,0x0FFC,0x03F8,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [@]
0x0000,0x0000,0x0000,0x03E0,0x03E0,0x07F0,0x07F0,0x07F0,0x0F78,0x0F78,0x0E7C,0x1E3C,0x1E3C,0x3C3E,0x3FFE,0x3FFF,0x781F,0x780F,0xF00F,0xF007,0xF007,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [A]
0x0000,0x0000,0x0000,0x3FF8,0x3FFC,0x3C3E,0x3C1E,0x3C1E,0x3C1E,0x3C3E,0x3C7C,0x3FF0,0x3FF8,0x3C7E,0x3C1F,0x3C1F,0x3C0F,0x3C0F,0x3C1F,0x3FFE,0x3FF8,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [B]
0x0000,0x0000,0x0000,0x01FF,0x07FF,0x1F87,0x3E00,0x3C00,0x7C00,0x7800,0x7800,0x7800,0x7800,0x7800,0x7C00,0x7C00,0x3E00,0x3F00,0x1F83,0x07FF,0x01FF,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [C]
0x0000,0x0000,0x0000,0x7FF0,0x7FFC,0x787E,0x781F,0x781F,0x780F,0x780F,0x780F,0x780F,0x780F,0x780F,0x780F,0x780F,0x781F,0x781E,0x787E,0x7FF8,0x7FE0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [D]
0x0000,0x0000,0x0000,0x3FFF,0x3FFF,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3FFE,0x3FFE,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3FFF,0x3FFF,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [E]
0x0000,0x0000,0x0000,0x1FFF,0x1FFF,0x1E00,0x1E00,0x1E00,0x1E00,0x1E00,0x1E00,0x1FFF,0x1FFF,0x1E00,0x1E00,0x1E00,0x1E00,0x1E00,0x1E00,0x1E00,0x1E00,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [F]
0x0000,0x0000,0x0000,0x03FE,0x0FFF,0x1F87,0x3E00,0x7C00,0x7C00,0x7800,0xF800,0xF800,0xF87F,0xF87F,0x780F,0x7C0F,0x7C0F,0x3E0F,0x1F8F,0x0FFF,0x03FE,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [G]
0x0000,0x0000,0x0000,0x7C1F,0x7C1F,0x7C1F,0x7C1F,0x7C1F,0x7C1F,0x7C1F,0x7C1F,0x7FFF,0x7FFF,0x7C1F,0x7C1F,0x7C1F,0x7C1F,0x7C1F,0x7C1F,0x7C1F,0x7C1F,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [H]
0x0000,0x0000,0x0000,0x3FFF,0x3FFF,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x3FFF,0x3FFF,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [I]
0x0000,0x0000,0x0000,0x1FFC,0x1FFC,0x007C,0x007C,0x007C,0x007C,0x007C,0x007C,0x007C,0x007C,0x007C,0x007C,0x007C,0x0078,0x0078,0x38F8,0x3FF0,0x3FC0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [J]
0x0000,0x0000,0x0000,0x3C1F,0x3C1E,0x3C3C,0x3C78,0x3CF0,0x3DE0,0x3FE0,0x3FC0,0x3F80,0x3FC0,0x3FE0,0x3DF0,0x3CF0,0x3C78,0x3C7C,0x3C3E,0x3C1F,0x3C0F,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [K]
0x0000,0x0000,0x0000,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3FFF,0x3FFF,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [L]
0x0000,0x0000,0x0000,0xF81F,0xFC1F,0xFC1F,0xFE3F,0xFE3F,0xFE3F,0xFF7F,0xFF77,0xFF77,0xF7F7,0xF7E7,0xF3E7,0xF3E7,0xF3C7,0xF007,0xF007,0xF007,0xF007,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [M]
0x0000,0x0000,0x0000,0x7C0F,0x7C0F,0x7E0F,0x7F0F,0x7F0F,0x7F8F,0x7F8F,0x7FCF,0x7BEF,0x79EF,0x79FF,0x78FF,0x78FF,0x787F,0x783F,0x783F,0x781F,0x781F,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [N]
0x0000,0x0000,0x0000,0x07F0,0x1FFC,0x3E3E,0x7C1F,0x780F,0x780F,0xF80F,0xF80F,0xF80F,0xF80F,0xF80F,0xF80F,0x780F,0x780F,0x7C1F,0x3E3E,0x1FFC,0x07F0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [O]
0x0000,0x0000,0x0000,0x3FFC,0x3FFF,0x3E1F,0x3E0F,0x3E0F,0x3E0F,0x3E0F,0x3E1F,0x3E3F,0x3FFC,0x3FF0,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x3E00,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [P]
0x0000,0x0000,0x0000,0x07F0,0x1FFC,0x3E3E,0x7C1F,0x780F,0x780F,0xF80F,0xF80F,0xF80F,0xF80F,0xF80F,0xF80F,0x780F,0x780F,0x7C1F,0x3E3E,0x1FFC,0x07F8,0x007C,0x003F,0x000F,0x0003,0x0000, // Ascii = [Q]
0x0000,0x0000,0x0000,0x3FF0,0x3FFC,0x3C7E,0x3C3E,0x3C1E,0x3C1E,0x3C3E,0x3C3C,0x3CFC,0x3FF0,0x3FE0,0x3DF0,0x3CF8,0x3C7C,0x3C3E,0x3C1E,0x3C1F,0x3C0F,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [R]
0x0000,0x0000,0x0000,0x07FC,0x1FFE,0x3E0E,0x3C00,0x3C00,0x3C00,0x3E00,0x1FC0,0x0FF8,0x03FE,0x007F,0x001F,0x000F,0x000F,0x201F,0x3C3E,0x3FFC,0x1FF0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [S]
0x0000,0x0000,0x0000,0xFFFF,0xFFFF,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [T]
0x0000,0x0000,0x0000,0x7C0F,0x7C0F,0x7C0F,0x7C0F,0x7C0F,0x7C0F,0x7C0F,0x7C0F,0x7C0F,0x7C0F,0x7C0F,0x7C0F,0x7C0F,0x3C1E,0x3C1E,0x3E3E,0x1FFC,0x07F0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [U]
0x0000,0x0000,0x0000,0xF007,0xF007,0xF807,0x780F,0x7C0F,0x3C1E,0x3C1E,0x3E1E,0x1E3C,0x1F3C,0x1F78,0x0F78,0x0FF8,0x07F0,0x07F0,0x07F0,0x03E0,0x03E0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [V]
0x0000,0x0000,0x0000,0xE003,0xF003,0xF003,0xF007,0xF3E7,0xF3E7,0xF3E7,0x73E7,0x7BF7,0x7FF7,0x7FFF,0x7F7F,0x7F7F,0x7F7E,0x3F7E,0x3E3E,0x3E3E,0x3E3E,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [W]
0x0000,0x0000,0x0000,0xF807,0x7C0F,0x3E1E,0x3E3E,0x1F3C,0x0FF8,0x07F0,0x07E0,0x03E0,0x03E0,0x07F0,0x0FF8,0x0F7C,0x1E7C,0x3C3E,0x781F,0x780F,0xF00F,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [X]
0x0000,0x0000,0x0000,0xF807,0x7807,0x7C0F,0x3C1E,0x3E1E,0x1F3C,0x0F78,0x0FF8,0x07F0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [Y]
0x0000,0x0000,0x0000,0x7FFF,0x7FFF,0x000F,0x001F,0x003E,0x007C,0x00F8,0x00F0,0x01E0,0x03E0,0x07C0,0x0F80,0x0F00,0x1E00,0x3E00,0x7C00,0x7FFF,0x7FFF,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [Z]
0x07FF,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x07FF,0x07FF,0x0000, // Ascii = [[]
0x7800,0x7800,0x3C00,0x3C00,0x1E00,0x1E00,0x0F00,0x0F00,0x0780,0x0780,0x03C0,0x03C0,0x01E0,0x01E0,0x00F0,0x00F0,0x0078,0x0078,0x003C,0x003C,0x001E,0x001E,0x000F,0x000F,0x0007,0x0000, // Ascii = [\]
0x7FF0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x7FF0,0x7FF0,0x0000, // Ascii = []]
0x00C0,0x01C0,0x01C0,0x03E0,0x03E0,0x07F0,0x07F0,0x0778,0x0F78,0x0F38,0x1E3C,0x1E3C,0x3C1E,0x3C1E,0x380F,0x780F,0x7807,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [^]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0xFFFF,0xFFFF,0x0000,0x0000,0x0000, // Ascii = [_]
0x00F0,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [`]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0FF8,0x3FFC,0x3C7C,0x003E,0x003E,0x003E,0x07FE,0x1FFE,0x3E3E,0x7C3E,0x783E,0x7C3E,0x7C7E,0x3FFF,0x1FCF,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [a]
0x3C00,0x3C00,0x3C00,0x3C00,0x3C00,0x3C00,0x3DF8,0x3FFE,0x3F3E,0x3E1F,0x3C0F,0x3C0F,0x3C0F,0x3C0F,0x3C0F,0x3C0F,0x3C1F,0x3C1E,0x3F3E,0x3FFC,0x3BF0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [b]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x03FE,0x0FFF,0x1F87,0x3E00,0x3E00,0x3C00,0x7C00,0x7C00,0x7C00,0x3C00,0x3E00,0x3E00,0x1F87,0x0FFF,0x03FE,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [c]
0x001F,0x001F,0x001F,0x001F,0x001F,0x001F,0x07FF,0x1FFF,0x3E3F,0x3C1F,0x7C1F,0x7C1F,0x7C1F,0x781F,0x781F,0x7C1F,0x7C1F,0x3C3F,0x3E7F,0x1FFF,0x0FDF,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [d]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x03F8,0x0FFC,0x1F3E,0x3E1E,0x3C1F,0x7C1F,0x7FFF,0x7FFF,0x7C00,0x7C00,0x3C00,0x3E00,0x1F07,0x0FFF,0x03FE,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [e]
0x01FF,0x03E1,0x03C0,0x07C0,0x07C0,0x07C0,0x7FFF,0x7FFF,0x07C0,0x07C0,0x07C0,0x07C0,0x07C0,0x07C0,0x07C0,0x07C0,0x07C0,0x07C0,0x07C0,0x07C0,0x07C0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [f]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x07EF,0x1FFF,0x3E7F,0x3C1F,0x7C1F,0x7C1F,0x781F,0x781F,0x781F,0x7C1F,0x7C1F,0x3C3F,0x3E7F,0x1FFF,0x0FDF,0x001E,0x001E,0x001E,0x387C,0x3FF8, // Ascii = [g]
0x3C00,0x3C00,0x3C00,0x3C00,0x3C00,0x3C00,0x3DFC,0x3FFE,0x3F9E,0x3F1F,0x3E1F,0x3C1F,0x3C1F,0x3C1F,0x3C1F,0x3C1F,0x3C1F,0x3C1F,0x3C1F,0x3C1F,0x3C1F,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [h]
0x01F0,0x01F0,0x0000,0x0000,0x0000,0x0000,0x7FE0,0x7FE0,0x01E0,0x01E0,0x01E0,0x01E0,0x01E0,0x01E0,0x01E0,0x01E0,0x01E0,0x01E0,0x01E0,0x01E0,0x01E0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [i]
0x00F8,0x00F8,0x0000,0x0000,0x0000,0x0000,0x3FF8,0x3FF8,0x00F8,0x00F8,0x00F8,0x00F8,0x00F8,0x00F8,0x00F8,0x00F8,0x00F8,0x00F8,0x00F8,0x00F8,0x00F8,0x00F8,0x00F8,0x00F0,0x71F0,0x7FE0, // Ascii = [j]
0x3C00,0x3C00,0x3C00,0x3C00,0x3C00,0x3C00,0x3C1F,0x3C3E,0x3C7C,0x3CF8,0x3DF0,0x3DE0,0x3FC0,0x3FC0,0x3FE0,0x3DF0,0x3CF8,0x3C7C,0x3C3E,0x3C1F,0x3C1F,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [k]
0x7FF0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x01F0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [l]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0xF79E,0xFFFF,0xFFFF,0xFFFF,0xFBE7,0xF9E7,0xF1C7,0xF1C7,0xF1C7,0xF1C7,0xF1C7,0xF1C7,0xF1C7,0xF1C7,0xF1C7,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [m]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x3DFC,0x3FFE,0x3F9E,0x3F1F,0x3E1F,0x3C1F,0x3C1F,0x3C1F,0x3C1F,0x3C1F,0x3C1F,0x3C1F,0x3C1F,0x3C1F,0x3C1F,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [n]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x07F0,0x1FFC,0x3E3E,0x3C1F,0x7C1F,0x780F,0x780F,0x780F,0x780F,0x780F,0x7C1F,0x3C1F,0x3E3E,0x1FFC,0x07F0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [o]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x3DF8,0x3FFE,0x3F3E,0x3E1F,0x3C0F,0x3C0F,0x3C0F,0x3C0F,0x3C0F,0x3C0F,0x3C1F,0x3E1E,0x3F3E,0x3FFC,0x3FF8,0x3C00,0x3C00,0x3C00,0x3C00,0x3C00, // Ascii = [p]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x07EE,0x1FFE,0x3E7E,0x3C1E,0x7C1E,0x781E,0x781E,0x781E,0x781E,0x781E,0x7C1E,0x7C3E,0x3E7E,0x1FFE,0x0FDE,0x001E,0x001E,0x001E,0x001E,0x001E, // Ascii = [q]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x1F7F,0x1FFF,0x1FE7,0x1FC7,0x1F87,0x1F00,0x1F00,0x1F00,0x1F00,0x1F00,0x1F00,0x1F00,0x1F00,0x1F00,0x1F00,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [r]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x07FC,0x1FFE,0x1E0E,0x3E00,0x3E00,0x3F00,0x1FE0,0x07FC,0x00FE,0x003E,0x001E,0x001E,0x3C3E,0x3FFC,0x1FF0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [s]
0x0000,0x0000,0x0000,0x0780,0x0780,0x0780,0x7FFF,0x7FFF,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x0780,0x07C0,0x03FF,0x01FF,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [t]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x3C1E,0x3C1E,0x3C1E,0x3C1E,0x3C1E,0x3C1E,0x3C1E,0x3C1E,0x3C1E,0x3C1E,0x3C3E,0x3C7E,0x3EFE,0x1FFE,0x0FDE,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [u]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0xF007,0x780F,0x780F,0x3C1E,0x3C1E,0x3E1E,0x1E3C,0x1E3C,0x0F78,0x0F78,0x0FF0,0x07F0,0x07F0,0x03E0,0x03E0,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [v]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0xF003,0xF1E3,0xF3E3,0xF3E7,0xF3F7,0xF3F7,0x7FF7,0x7F77,0x7F7F,0x7F7F,0x7F7F,0x3E3E,0x3E3E,0x3E3E,0x3E3E,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [w]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x7C0F,0x3E1E,0x3E3C,0x1F3C,0x0FF8,0x07F0,0x07F0,0x03E0,0x07F0,0x07F8,0x0FF8,0x1E7C,0x3E3E,0x3C1F,0x781F,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [x]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0xF807,0x780F,0x7C0F,0x3C1E,0x3C1E,0x1E3C,0x1E3C,0x1F3C,0x0F78,0x0FF8,0x07F0,0x07F0,0x03E0,0x03E0,0x03C0,0x03C0,0x03C0,0x0780,0x0F80,0x7F00, // Ascii = [y]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x3FFF,0x3FFF,0x001F,0x003E,0x007C,0x00F8,0x01F0,0x03E0,0x07C0,0x0F80,0x1F00,0x1E00,0x3C00,0x7FFF,0x7FFF,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [z]
0x01FE,0x03E0,0x03C0,0x03C0,0x03C0,0x03C0,0x01E0,0x01E0,0x01E0,0x01C0,0x03C0,0x3F80,0x3F80,0x03C0,0x01C0,0x01E0,0x01E0,0x01E0,0x03C0,0x03C0,0x03C0,0x03C0,0x03E0,0x01FE,0x007E,0x0000, // Ascii = [{]
0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x0000, // Ascii = [|]
0x3FC0,0x03E0,0x01E0,0x01E0,0x01E0,0x01E0,0x01C0,0x03C0,0x03C0,0x01C0,0x01E0,0x00FE,0x00FE,0x01E0,0x01C0,0x03C0,0x03C0,0x01C0,0x01E0,0x01E0,0x01E0,0x01E0,0x03E0,0x3FC0,0x3F00,0x0000, // Ascii = [}]
0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x3F07,0x7FC7,0x73E7,0xF1FF,0xF07E,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, // Ascii = [~]
};

void font10x16_get_symbol_table (uint8_t symbol, uint8_t **symbol_table)
{
    *symbol_table = (uint8_t*)(&font10x16_table[symbol][0]);

    return;
}

void font10x16_get_symbol_width (uint8_t const * const symbol_table, uint8_t * const symbol_width)
{
    *symbol_width = symbol_table[0];

    return;
}

void font10x16_get_symbol_height (uint8_t const * const symbol_table, uint8_t * const symbol_height)
{
    *symbol_height = symbol_table[1];

    return;
}

void font16x26_get_table (uint16_t **table)
{
    *table = (uint16_t*)font16x26_table;

    return;
}
