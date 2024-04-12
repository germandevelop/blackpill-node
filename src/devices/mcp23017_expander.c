/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "mcp23017_expander.h"

#include <stddef.h>
#include <assert.h>

#include "std_error/std_error.h"


#define MCP23017_DEVICE_ADDRESS 0x20

#define MCP23017_PORT_A_DIRECTION_REGISTER_ADDRESS 0x00
#define MCP23017_PORT_B_DIRECTION_REGISTER_ADDRESS 0x01

#define MCP23017_PORT_A_POLARITY_REGISTER_ADDRESS 0x02
#define MCP23017_PORT_B_POLARITY_REGISTER_ADDRESS 0x03

#define MCP23017_PORT_A_ENABLE_INTERRUPT_REGISTER_ADDRESS 0x04
#define MCP23017_PORT_B_ENABLE_INTERRUPT_REGISTER_ADDRESS 0x05

#define MCP23017_PORT_A_DEFAULT_VALUE_REGISTER_ADDRESS 0x06
#define MCP23017_PORT_B_DEFAULT_VALUE_REGISTER_ADDRESS 0x07

#define MCP23017_PORT_A_INTERRUPT_CONTROL_REGISTER_ADDRESS 0x08
#define MCP23017_PORT_B_INTERRUPT_CONTROL_REGISTER_ADDRESS 0x09

#define MCP23017_PORT_A_PULLUP_REGISTER_ADDRESS 0x0C
#define MCP23017_PORT_B_PULLUP_REGISTER_ADDRESS 0x0D

#define MCP23017_CONFIGURATION_REGISTER_ADDRESS 0x0A

#define MCP23017_PORT_A_INTERRUPT_FLAG_REGISTER_ADDRESS 0x0E
#define MCP23017_PORT_B_INTERRUPT_FLAG_REGISTER_ADDRESS 0x0F

#define MCP23017_PORT_A_INTERRUPT_CAPTURED_REGISTER_ADDRESS 0x10
#define MCP23017_PORT_B_INTERRUPT_CAPTURED_REGISTER_ADDRESS 0x11

#define MCP23017_PORT_A_GPIO_REGISTER_ADDRESS 0x12
#define MCP23017_PORT_B_GPIO_REGISTER_ADDRESS 0x13

#define MCP23017_PORT_A_OUTPUT_LATCH_REGISTER_ADDRESS 0x14
#define MCP23017_PORT_B_OUTPUT_LATCH_REGISTER_ADDRESS 0x15



int mcp23017_expander_init (mcp23017_expander_t * const self,
                            mcp23017_expander_config_t const * const init_config,
                            std_error_t * const error)
{
    mcp23017_expander_set_config(self, init_config);

    // Initial values of registers according to the datasheet
    self->image.port_direction_reg[PORT_A]      = 0xFF;
    self->image.port_direction_reg[PORT_B]      = 0xFF;
    self->image.port_out_reg[PORT_A]            = 0x00;
    self->image.port_out_reg[PORT_B]            = 0x00;
    self->image.port_int_control_reg[PORT_A]    = 0x00;
    self->image.port_int_control_reg[PORT_B]    = 0x00;
    self->image.port_int_cmp_mode_reg[PORT_A]   = 0x00;
    self->image.port_int_cmp_mode_reg[PORT_B]   = 0x00;
    self->image.port_int_cmp_value_reg[PORT_A]  = 0x00;
    self->image.port_int_cmp_value_reg[PORT_B]  = 0x00;
    self->image.port_int_polarity_reg[PORT_A]   = 0x00;
    self->image.port_int_polarity_reg[PORT_B]   = 0x00;
    self->image.port_int_pullup_reg[PORT_A]     = 0x00;
    self->image.port_int_pullup_reg[PORT_B]     = 0x00;

    self->image.config_reg = 0x00;

    // Unimplemented(bit 0): Read as "0"
    self->image.config_reg |= (0 << 0);

    // INTPOL(bit 1): This bit sets the polarity of the INT output pin
    // 1 = Active-high
    // 0 = Active-low
    self->image.config_reg |= (1 << 1);

    // ODR(bit 2): Configures the INT pin as an open-drain output
    // 1 = Open-drain output (overrides the INTPOL bit)
    // 0 = Active driver output (Push-Pull) (INTPOL bit sets the polarity)
    self->image.config_reg |= (0 << 2);

    // HAEN(bit 3): Hardware Address Enable bit (MCP23S17 only)
    // 1 = Enables the MCP23S17 address pins
    // 0 = Disables the MCP23S17 address pins
    self->image.config_reg |= (0 << 3);

    // DISSLW(bit 4): Slew Rate control bit for SDA output
    // 1 = Slew rate disabled
    // 0 = Slew rate enabled
    self->image.config_reg |= (1 << 4);

    // SEQOP(bit 5): Sequential Operation mode bit
    // 1 = Sequential operation disabled, address pointer does not increment
    // 0 = Sequential operation enabled, address pointer increments
    self->image.config_reg |= (0 << 5);

    // MIRROR(bit 6): INT Pins Mirror bit
    // 1 = The INT pins are internally connected
    // 0 = The INT pins are not connected. INTA is associated with PORTA and INTB is associated with PORTB
    self->image.config_reg |= (1 << 6);

    // BANK(bit 7): Controls how the registers are addressed
    // 1 = The registers associated with each port are separated into different banks
    // 0 = The registers are in the same bank (addresses are sequential)
    self->image.config_reg |= (0 << 7);

    self->config.i2c_lock_callback();
    int exit_code = self->config.write_i2c_callback(MCP23017_DEVICE_ADDRESS, MCP23017_CONFIGURATION_REGISTER_ADDRESS, sizeof(uint8_t), &self->image.config_reg, sizeof(self->image.config_reg), self->config.i2c_timeout_ms, error);
    self->config.i2c_unlock_callback();

    return exit_code;
}

void mcp23017_expander_set_config ( mcp23017_expander_t * const self,
                                    mcp23017_expander_config_t const * const config)
{
    assert(self                         != NULL);
    assert(config                       != NULL);
    assert(config->i2c_lock_callback    != NULL);
    assert(config->i2c_unlock_callback  != NULL);
    assert(config->read_i2c_callback    != NULL);
    assert(config->write_i2c_callback   != NULL);

    self->config = *config;

    return;
}

int mcp23017_expander_set_port_direction (  mcp23017_expander_t * const self,
                                            mcp23017_expander_port_t port,
                                            mcp23017_expander_direction_t direction,
                                            std_error_t * const error)
{
    assert(self != NULL);

    const uint16_t register_address[] =
    {
        [PORT_A] = MCP23017_PORT_A_DIRECTION_REGISTER_ADDRESS,
        [PORT_B] = MCP23017_PORT_B_DIRECTION_REGISTER_ADDRESS
    };

    if (direction == OUTPUT_DIRECTION)
    {
        self->image.port_direction_reg[port] = 0x00;
    }
    else
    {
        self->image.port_direction_reg[port] = 0xFF;
    }

    self->config.i2c_lock_callback();
    int exit_code = self->config.write_i2c_callback(MCP23017_DEVICE_ADDRESS, register_address[port], sizeof(uint8_t), &self->image.port_direction_reg[port], sizeof(uint8_t), self->config.i2c_timeout_ms, error);
    self->config.i2c_unlock_callback();

    return exit_code;
}

int mcp23017_expander_set_pin_direction (mcp23017_expander_t * const self,
                                        mcp23017_expander_port_t port,
                                        mcp23017_expander_pin_t pin,
                                        mcp23017_expander_direction_t direction,
                                        std_error_t * const error)
{
    assert(self != NULL);

    const uint16_t register_address[] =
    {
        [PORT_A] = MCP23017_PORT_A_DIRECTION_REGISTER_ADDRESS,
        [PORT_B] = MCP23017_PORT_B_DIRECTION_REGISTER_ADDRESS
    };

    if (direction == OUTPUT_DIRECTION)
    {
        self->image.port_direction_reg[port] &= ~(1 << pin);
    }
    else
    {
        self->image.port_direction_reg[port] |= (1 << pin);
    }

    self->config.i2c_lock_callback();
    int exit_code = self->config.write_i2c_callback(MCP23017_DEVICE_ADDRESS, register_address[port], sizeof(uint8_t), &self->image.port_direction_reg[port], sizeof(uint8_t), self->config.i2c_timeout_ms, error);
    self->config.i2c_unlock_callback();

    return exit_code;
}

int mcp23017_expander_set_port_out (mcp23017_expander_t * const self,
                                    mcp23017_expander_port_t port,
                                    mcp23017_expander_gpio_t gpio,
                                    std_error_t * const error)
{
    assert(self != NULL);

    const uint16_t register_address[] =
    {
        [PORT_A] = MCP23017_PORT_A_OUTPUT_LATCH_REGISTER_ADDRESS,
        [PORT_B] = MCP23017_PORT_B_OUTPUT_LATCH_REGISTER_ADDRESS
    };

    if (gpio == LOW_GPIO)
    {
        self->image.port_out_reg[port] = 0x00;
    }
    else
    {
        self->image.port_out_reg[port] = 0xFF;
    }

    self->config.i2c_lock_callback();
    int exit_code = self->config.write_i2c_callback(MCP23017_DEVICE_ADDRESS, register_address[port], sizeof(uint8_t), &self->image.port_out_reg[port], sizeof(uint8_t), self->config.i2c_timeout_ms, error);
    self->config.i2c_unlock_callback();

    return exit_code;
}

int mcp23017_expander_set_pin_out ( mcp23017_expander_t * const self,
                                    mcp23017_expander_port_t port,
                                    mcp23017_expander_pin_t pin,
                                    mcp23017_expander_gpio_t gpio,
                                    std_error_t * const error)
{
    assert(self != NULL);

    const uint16_t register_address[] =
    {
        [PORT_A] = MCP23017_PORT_A_OUTPUT_LATCH_REGISTER_ADDRESS,
        [PORT_B] = MCP23017_PORT_B_OUTPUT_LATCH_REGISTER_ADDRESS
    };

    if (gpio == LOW_GPIO)
    {
        self->image.port_out_reg[port] &= ~(1 << pin);
    }
    else
    {
        self->image.port_out_reg[port] |= (1 << pin);
    }

    self->config.i2c_lock_callback();
    int exit_code = self->config.write_i2c_callback(MCP23017_DEVICE_ADDRESS, register_address[port], sizeof(uint8_t), &self->image.port_out_reg[port], sizeof(uint8_t), self->config.i2c_timeout_ms, error);
    self->config.i2c_unlock_callback();

    return exit_code;
}

int mcp23017_expander_get_port_in ( mcp23017_expander_t const * const self,
                                    mcp23017_expander_port_t port,
                                    uint8_t * const port_in,
                                    std_error_t * const error)
{
    assert(self != NULL);
    assert(port_in != NULL);

    const uint16_t register_address[] =
    {
        [PORT_A] = MCP23017_PORT_A_GPIO_REGISTER_ADDRESS,
        [PORT_B] = MCP23017_PORT_B_GPIO_REGISTER_ADDRESS
    };

    uint8_t current_register_value[1];

    self->config.i2c_lock_callback();
    int exit_code = self->config.read_i2c_callback(MCP23017_DEVICE_ADDRESS, register_address[port], sizeof(uint8_t), current_register_value, sizeof(current_register_value), self->config.i2c_timeout_ms, error);
    self->config.i2c_unlock_callback();

    *port_in = current_register_value[0];

    return exit_code;
}

int mcp23017_expander_set_pin_int ( mcp23017_expander_t * const self,
                                    mcp23017_expander_port_t port,
                                    mcp23017_expander_pin_t pin,
                                    mcp23017_expander_int_config_t const * const config,
                                    std_error_t * const error)
{
    assert(self != NULL);

    int exit_code = STD_FAILURE;

    self->config.i2c_lock_callback();

    // Configure 'Interrupt Control Register'
    {
        const uint16_t register_address[] =
        {
            [PORT_A] = MCP23017_PORT_A_INTERRUPT_CONTROL_REGISTER_ADDRESS,
            [PORT_B] = MCP23017_PORT_B_INTERRUPT_CONTROL_REGISTER_ADDRESS
        };

        if (config->cmp_mode == DISABLE_COMPARISON)
        {
            self->image.port_int_cmp_mode_reg[port] &= ~(1 << pin);
        }
        else
        {
            self->image.port_int_cmp_mode_reg[port] |= (1 << pin);
        }

        exit_code = self->config.write_i2c_callback(MCP23017_DEVICE_ADDRESS, register_address[port], sizeof(uint8_t), &self->image.port_int_cmp_mode_reg[port], sizeof(uint8_t), self->config.i2c_timeout_ms, error);

        if (exit_code != STD_SUCCESS)
        {
            self->config.i2c_unlock_callback();

            return exit_code;
        }
    }

    // Configure 'Interrupt Compare Register'
    {
        const uint16_t register_address[] =
        {
            [PORT_A] = MCP23017_PORT_A_DEFAULT_VALUE_REGISTER_ADDRESS,
            [PORT_B] = MCP23017_PORT_B_DEFAULT_VALUE_REGISTER_ADDRESS
        };

        if (config->cmp_value == LOW_COMPARISON_VALUE)
        {
            self->image.port_int_cmp_value_reg[port] &= ~(1 << pin);
        }
        else
        {
            self->image.port_int_cmp_value_reg[port] |= (1 << pin);
        }

        exit_code = self->config.write_i2c_callback(MCP23017_DEVICE_ADDRESS, register_address[port], sizeof(uint8_t), &self->image.port_int_cmp_value_reg[port], sizeof(uint8_t), self->config.i2c_timeout_ms, error);

        if (exit_code != STD_SUCCESS)
        {
            self->config.i2c_unlock_callback();

            return exit_code;
        }
    }

    // Configure 'Interrupt Polarity Register'
    {
        const uint16_t register_address[] =
        {
            [PORT_A] = MCP23017_PORT_A_POLARITY_REGISTER_ADDRESS,
            [PORT_B] = MCP23017_PORT_B_POLARITY_REGISTER_ADDRESS
        };

        if (config->polarity == SAME_POLARITY)
        {
            self->image.port_int_polarity_reg[port] &= ~(1 << pin);
        }
        else
        {
            self->image.port_int_polarity_reg[port] |= (1 << pin);
        }

        exit_code = self->config.write_i2c_callback(MCP23017_DEVICE_ADDRESS, register_address[port], sizeof(uint8_t), &self->image.port_int_polarity_reg[port], sizeof(uint8_t), self->config.i2c_timeout_ms, error);

        if (exit_code != STD_SUCCESS)
        {
            self->config.i2c_unlock_callback();

            return exit_code;
        }
    }

    // Configure 'Interrupt Pullup Register'
    {
        const uint16_t register_address[] =
        {
            [PORT_A] = MCP23017_PORT_A_PULLUP_REGISTER_ADDRESS,
            [PORT_B] = MCP23017_PORT_B_PULLUP_REGISTER_ADDRESS
        };

        if (config->pullup == DISABLE_PULL_UP)
        {
            self->image.port_int_pullup_reg[port] &= ~(1 << pin);
        }
        else
        {
            self->image.port_int_pullup_reg[port] |= (1 << pin);
        }

        exit_code = self->config.write_i2c_callback(MCP23017_DEVICE_ADDRESS, register_address[port], sizeof(uint8_t), &self->image.port_int_pullup_reg[port], sizeof(uint8_t), self->config.i2c_timeout_ms, error);

        if (exit_code != STD_SUCCESS)
        {
            self->config.i2c_unlock_callback();

            return exit_code;
        }
    }

    // Configure 'Enable Interrupt Control Register'
    {
        const uint16_t register_address[] =
        {
            [PORT_A] = MCP23017_PORT_A_ENABLE_INTERRUPT_REGISTER_ADDRESS,
            [PORT_B] = MCP23017_PORT_B_ENABLE_INTERRUPT_REGISTER_ADDRESS
        };

        if (config->control == DISABLE_INTERRUPT)
        {
            self->image.port_int_control_reg[port] &= ~(1 << pin);
        }
        else
        {
            self->image.port_int_control_reg[port] |= (1 << pin);
        }

        exit_code = self->config.write_i2c_callback(MCP23017_DEVICE_ADDRESS, register_address[port], sizeof(uint8_t), &self->image.port_int_control_reg[port], sizeof(uint8_t), self->config.i2c_timeout_ms, error);
    }

    self->config.i2c_unlock_callback();

    return exit_code;
}


int mcp23017_expander_get_int_flag (mcp23017_expander_t const * const self,
                                    mcp23017_expander_port_t port,
                                    uint8_t * const port_flag,
                                    std_error_t * const error)
{
    assert(self != NULL);
    assert(self->config.read_i2c_callback != NULL);
    assert(port_flag != NULL);

    uint16_t register_address[] =
    {
        [PORT_A] = MCP23017_PORT_A_INTERRUPT_FLAG_REGISTER_ADDRESS,
        [PORT_B] = MCP23017_PORT_B_INTERRUPT_FLAG_REGISTER_ADDRESS
    };

    uint8_t flag_value[1];

    self->config.i2c_lock_callback();
    int exit_code = self->config.read_i2c_callback(MCP23017_DEVICE_ADDRESS, register_address[port], sizeof(uint8_t), flag_value, sizeof(flag_value), self->config.i2c_timeout_ms, error);
    self->config.i2c_unlock_callback();

    *port_flag = flag_value[0];

    return exit_code;
}

int mcp23017_expander_get_int_capture ( mcp23017_expander_t * const self,
                                        mcp23017_expander_port_t port,
                                        uint8_t * const port_capture,
                                        std_error_t * const error)
{
    assert(self != NULL);
    assert(self->config.read_i2c_callback != NULL);
    assert(port_capture != NULL);

    uint16_t register_address[] =
    {
        [PORT_A] = MCP23017_PORT_A_INTERRUPT_CAPTURED_REGISTER_ADDRESS,
        [PORT_B] = MCP23017_PORT_B_INTERRUPT_CAPTURED_REGISTER_ADDRESS
    };

    uint8_t capture_value[1];

    self->config.i2c_lock_callback();
    const int exit_code = self->config.read_i2c_callback(MCP23017_DEVICE_ADDRESS, register_address[port], sizeof(uint8_t), capture_value, sizeof(capture_value), self->config.i2c_timeout_ms, error);
    self->config.i2c_unlock_callback();

    *port_capture = capture_value[0];

    return exit_code;
}
