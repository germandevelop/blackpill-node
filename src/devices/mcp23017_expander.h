/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef MCP23017_EXPANDER_H
#define MCP23017_EXPANDER_H

#include <stdint.h>

typedef struct std_error std_error_t;

typedef int (*mcp23017_expander_i2c_callback_t) (uint16_t device_address, uint16_t register_address, uint16_t register_size, uint8_t *array, uint16_t array_size, uint32_t timeout_ms, std_error_t * const error);

typedef struct mcp23017_expander_config
{
    mcp23017_expander_i2c_callback_t read_i2c_callback;
    mcp23017_expander_i2c_callback_t write_i2c_callback;
    uint32_t i2c_timeout_ms;

} mcp23017_expander_config_t;

typedef struct mcp23017_expander mcp23017_expander_t;

typedef enum mcp23017_expander_port
{
    PORT_A = 0,
    PORT_B = 1

} mcp23017_expander_port_t;

typedef enum mcp23017_expander_pin
{
    PIN_0 = 0,
    PIN_1 = 1,
    PIN_2 = 2,
    PIN_3 = 3,
    PIN_4 = 4,
    PIN_5 = 5,
    PIN_6 = 6,
    PIN_7 = 7

} mcp23017_expander_pin_t;

typedef enum mcp23017_expander_direction
{
    OUTPUT_DIRECTION    = 0,
    INPUT_DIRECTION     = 1

} mcp23017_expander_direction_t;

typedef enum mcp23017_expander_gpio
{
    LOW_GPIO    = 0,
    HIGH_GPIO   = 1

} mcp23017_expander_gpio_t;

typedef enum mcp23017_expander_int_control
{
    DISABLE_INTERRUPT   = 0,
    ENABLE_INTERRUPT    = 1

} mcp23017_expander_int_control_t;

typedef enum mcp23017_expander_int_cmp_mode
{
    DISABLE_COMPARISON  = 0,    // Pin value is compared against the previous pin value - RISING and FALLING
    ENABLE_COMPARISON   = 1     // Pin value is compared against 'COMPARISON VALUE' - RISING or FALLING

} mcp23017_expander_int_cmp_mode_t;

typedef enum mcp23017_expander_int_cmp_value
{
    LOW_COMPARISON_VALUE    = 0, // RISING signal throws interrupt
    HIGH_COMPARISON_VALUE   = 1  // FALLING signal throws interrupt

} mcp23017_expander_int_cmp_value_t;

typedef enum mcp23017_expander_int_polarity
{
    SAME_POLARITY       = 0,
    INVERTED_POLARITY   = 1

} mcp23017_expander_int_polarity_t;

typedef enum mcp23017_expander_int_pullup
{
    DISABLE_PULL_UP = 0,
    ENABLE_PULL_UP  = 1 // 100kOhm

} mcp23017_expander_int_pullup_t;

typedef struct mcp23017_expander_int_config
{
    mcp23017_expander_int_control_t     control;
    mcp23017_expander_int_cmp_mode_t    cmp_mode;
    mcp23017_expander_int_cmp_value_t   cmp_value;
    mcp23017_expander_int_polarity_t    polarity;
    mcp23017_expander_int_pullup_t      pullup;

} mcp23017_expander_int_config_t;


#ifdef __cplusplus
extern "C" {
#endif

int mcp23017_expander_init (mcp23017_expander_t * const self,
                            mcp23017_expander_config_t const * const init_config,
                            std_error_t * const error);

void mcp23017_expander_set_config ( mcp23017_expander_t * const self,
                                    mcp23017_expander_config_t const * const config);

// Direction
int mcp23017_expander_set_port_direction (  mcp23017_expander_t * const self,
                                            mcp23017_expander_port_t port,
                                            mcp23017_expander_direction_t direction,
                                            std_error_t * const error);

int mcp23017_expander_set_pin_direction (mcp23017_expander_t * const self,
                                        mcp23017_expander_port_t port,
                                        mcp23017_expander_pin_t pin,
                                        mcp23017_expander_direction_t direction,
                                        std_error_t * const error);

// Output GPIO
int mcp23017_expander_set_port_out (mcp23017_expander_t * const self,
                                    mcp23017_expander_port_t port,
                                    mcp23017_expander_gpio_t gpio,
                                    std_error_t * const error);

int mcp23017_expander_set_pin_out ( mcp23017_expander_t * const self,
                                    mcp23017_expander_port_t port,
                                    mcp23017_expander_pin_t pin,
                                    mcp23017_expander_gpio_t gpio,
                                    std_error_t * const error);

// Input GPIO
int mcp23017_expander_get_port_in ( mcp23017_expander_t const * const self,
                                    mcp23017_expander_port_t port,
                                    uint8_t * const port_in,
                                    std_error_t * const error);

// Input GPIO - interrupt
int mcp23017_expander_set_pin_int ( mcp23017_expander_t * const self,
                                    mcp23017_expander_port_t port,
                                    mcp23017_expander_pin_t pin,
                                    mcp23017_expander_int_config_t const * const config,
                                    std_error_t * const error);

int mcp23017_expander_get_int_flag (mcp23017_expander_t const * const self,
                                    mcp23017_expander_port_t port,
                                    uint8_t * const port_flag,
                                    std_error_t * const error);

int mcp23017_expander_get_int_capture ( mcp23017_expander_t * const self,
                                        mcp23017_expander_port_t port,
                                        uint8_t * const port_capture,
                                        std_error_t * const error);

#ifdef __cplusplus
}
#endif



// Private
typedef struct mcp23017_expander
{
    mcp23017_expander_config_t config;

    // Register images
    uint8_t config_reg_image;
    uint8_t port_direction_reg_image[2];
    uint8_t port_out_reg_image[2];
    uint8_t port_int_control_reg_image[2];
    uint8_t port_int_cmp_mode_reg_image[2];
    uint8_t port_int_cmp_value_reg_image[2];
    uint8_t port_int_polarity_reg_image[2];
    uint8_t port_int_pullup_reg_image[2];

} mcp23017_expander_t;

#endif // MCP23017_EXPANDER_H
