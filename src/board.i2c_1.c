/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "board.i2c_1.h"

#include "stm32f4xx_hal.h"

#include "std_error/std_error.h"


#define I2C_DEFAULT_ERROR_TEXT  "I2C_2 error"


static I2C_HandleTypeDef i2c_1_handler;


static void board_i2c_1_msp_init (I2C_HandleTypeDef *i2c_handler);
static void board_i2c_1_msp_deinit (I2C_HandleTypeDef *i2c_handler);

int board_i2c_1_init (std_error_t * const error)
{
    i2c_1_handler.Instance              = I2C1;
    i2c_1_handler.MspInitCallback       = board_i2c_1_msp_init;
    i2c_1_handler.MspDeInitCallback     = board_i2c_1_msp_deinit;
    i2c_1_handler.Init.ClockSpeed       = 400000U;
    i2c_1_handler.Init.DutyCycle        = I2C_DUTYCYCLE_2;
    i2c_1_handler.Init.OwnAddress1      = 0U;
    i2c_1_handler.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    i2c_1_handler.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    i2c_1_handler.Init.OwnAddress2      = 0U;
    i2c_1_handler.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    i2c_1_handler.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;

    const HAL_StatusTypeDef status = HAL_I2C_Init(&i2c_1_handler);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, I2C_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

void board_i2c_1_deinit ()
{
    HAL_I2C_DeInit(&i2c_1_handler);

    return;
}

void board_i2c_1_enable_clock ()
{
    __HAL_RCC_I2C1_CLK_ENABLE();

    return;
}

void board_i2c_1_disable_clock ()
{
    __HAL_RCC_I2C1_CLK_DISABLE();

    return;
}

int board_i2c_1_write_register (uint16_t device_address,
                                uint16_t register_address,
                                uint16_t register_size,
                                uint8_t *array,
                                uint16_t array_size,
                                uint32_t timeout_ms,
                                std_error_t * const error)
{
    const uint16_t shifted_device_address = device_address << 1;

    const HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&i2c_1_handler, shifted_device_address, register_address, register_size, array, array_size, timeout_ms);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, I2C_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

int board_i2c_1_read_register (uint16_t device_address,
                                uint16_t register_address,
                                uint16_t register_size,
                                uint8_t *array,
                                uint16_t array_size,
                                uint32_t timeout_ms,
                                std_error_t * const error)
{
    const uint16_t shifted_device_address = device_address << 1;

    const HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&i2c_1_handler, shifted_device_address, register_address, register_size, array, array_size, timeout_ms);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, I2C_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

int board_i2c_1_write (uint16_t device_address,
                        uint8_t *array,
                        uint16_t array_size,
                        uint32_t timeout_ms,
                        std_error_t * const error)
{
    const uint16_t shifted_device_address = device_address << 1;

    const HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&i2c_1_handler, shifted_device_address, array, array_size, timeout_ms);

    if (status != HAL_OK)
    {
        std_error_catch_custom(error, (int)status, I2C_DEFAULT_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}


void board_i2c_1_msp_init (I2C_HandleTypeDef *i2c_handler)
{
    UNUSED(i2c_handler);

    // Peripheral clock enable
    __HAL_RCC_I2C1_CLK_SLEEP_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // I2C1 GPIO Configuration
    // PB8     ------> I2C1_SCL
    // PB9     ------> I2C1_SDA
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin         = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode        = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull        = GPIO_NOPULL;
    GPIO_InitStruct.Speed       = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate   = GPIO_AF4_I2C1;
        
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // I2C1 interrupt Init
    //HAL_NVIC_SetPriority(I2C1_EV_IRQn, 5U, 0U);
    //HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    //HAL_NVIC_SetPriority(I2C1_ER_IRQn, 5U, 0U);
    //HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);

    return;
}

void board_i2c_1_msp_deinit (I2C_HandleTypeDef *i2c_handler)
{
    UNUSED(i2c_handler);
    
    // Peripheral clock disable
    __HAL_RCC_I2C1_CLK_DISABLE();

    // I2C1 GPIO Configuration
    // PB8     ------> I2C1_SCL
    // PB9     ------> I2C1_SDA
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8 | GPIO_PIN_9);

    // I2C1 interrupt DeInit
    //HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
    //HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);

    return;
}
