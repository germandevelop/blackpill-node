/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include <gmock/gmock.h>

#include "device/mcp23017_expander.h"
#include "std_error/std_error.h"


class MockConfigI2C
{
    public:
        MOCK_METHOD(int, readI2C, (uint16_t device_address, uint16_t register_address, uint16_t register_size, uint8_t *array, uint16_t array_size, uint32_t timeout_ms, std_error_t * const error));
        MOCK_METHOD(int, writeI2C, (uint16_t device_address, uint16_t register_address, uint16_t register_size, uint8_t *array, uint16_t array_size, uint32_t timeout_ms, std_error_t * const error));
};

static testing::NiceMock<MockConfigI2C> *mockConfigI2C;

static int read_i2c_mock (uint16_t device_address, uint16_t register_address, uint16_t register_size, uint8_t *array, uint16_t array_size, uint32_t timeout_ms, std_error_t * const error)
{
    return mockConfigI2C->readI2C(device_address, register_address, register_size, array, array_size, timeout_ms, error);
}

static int write_i2c_mock (uint16_t device_address, uint16_t register_address, uint16_t register_size, uint8_t *array, uint16_t array_size, uint32_t timeout_ms, std_error_t * const error)
{
     return mockConfigI2C->writeI2C(device_address, register_address, register_size, array, array_size, timeout_ms, error);
}


class Mcp23017ExpanderTestFixture : public ::testing::Test
{
    protected:

        mcp23017_expander_t expander;
        std_error_t error;

        virtual void SetUp() override
        {
            mockConfigI2C = new testing::NiceMock<MockConfigI2C> {};

            std_error_init(&error);

            mcp23017_expander_config_t config;
            config.write_i2c_callback = write_i2c_mock;
            config.read_i2c_callback  = read_i2c_mock;
            config.i2c_timeout_ms     = 0U;

            mcp23017_expander_init(&expander, &config, &error);
        }

        virtual void TearDown() override
        {
            delete mockConfigI2C;
        }
};


TEST_F(Mcp23017ExpanderTestFixture, Init)
{
    // Arrange: create and set up a system under test
    mcp23017_expander_config_t init_config;
    init_config.write_i2c_callback = write_i2c_mock;
    init_config.read_i2c_callback  = read_i2c_mock;
    init_config.i2c_timeout_ms     = 0U;

    int expected_ret_val = STD_SUCCESS;
    uint8_t expected_config_val = (0 << 0) | (1 << 1) | (0 << 2) | (0 << 3) | (1 << 4) | (0 << 5) | (1 << 6) | (0 << 7);
    uint8_t expected_port_direction[2], expected_port_out[2];
    expected_port_direction[PORT_A] = 0xFF;
    expected_port_direction[PORT_B] = 0xFF;
    expected_port_out[PORT_A]       = 0x00;
    expected_port_out[PORT_B]       = 0x00;
    uint8_t expected_port_int_control[2], expected_port_int_cmp_mode[2], expected_port_int_cmp_value[2];
    uint8_t expected_port_int_polarity[2], expected_port_int_pullup[2];
    expected_port_int_control[PORT_A]   = 0x00;
    expected_port_int_control[PORT_B]   = 0x00;
    expected_port_int_cmp_mode[PORT_A]  = 0x00;
    expected_port_int_cmp_mode[PORT_B]  = 0x00;
    expected_port_int_cmp_value[PORT_A] = 0x00;
    expected_port_int_cmp_value[PORT_B] = 0x00;
    expected_port_int_polarity[PORT_A]  = 0x00;
    expected_port_int_polarity[PORT_B]  = 0x00;
    expected_port_int_pullup[PORT_A]    = 0x00;
    expected_port_int_pullup[PORT_B]    = 0x00;

    EXPECT_CALL(*mockConfigI2C, writeI2C(MCP23017_DEVICE_ADDRESS, MCP23017_CONFIGURATION_REGISTER_ADDRESS,
                testing::_, testing::Pointee(expected_config_val), testing::_, testing::_, testing::_))
                .Times(1)
                .WillRepeatedly(testing::Return(STD_SUCCESS));

    // Act: poke the system under test
    mcp23017_expander_t init_expander;
    int ret_val = mcp23017_expander_init(&init_expander, &init_config, &error);

    // Assert: make unit test pass or fail
    ASSERT_EQ(ret_val, expected_ret_val);
    EXPECT_EQ(init_expander.config_reg_image, expected_config_val);
    EXPECT_EQ(init_expander.port_direction_reg_image[PORT_A], expected_port_direction[PORT_A]);
    EXPECT_EQ(init_expander.port_direction_reg_image[PORT_B], expected_port_direction[PORT_B]);
    EXPECT_EQ(init_expander.port_out_reg_image[PORT_A], expected_port_out[PORT_A]);
    EXPECT_EQ(init_expander.port_out_reg_image[PORT_B], expected_port_out[PORT_B]);
    EXPECT_EQ(init_expander.port_int_control_reg_image[PORT_A], expected_port_int_control[PORT_A]);
    EXPECT_EQ(init_expander.port_int_control_reg_image[PORT_B], expected_port_int_control[PORT_B]);
    EXPECT_EQ(init_expander.port_int_cmp_mode_reg_image[PORT_A], expected_port_int_cmp_mode[PORT_A]);
    EXPECT_EQ(init_expander.port_int_cmp_mode_reg_image[PORT_B], expected_port_int_cmp_mode[PORT_B]);
    EXPECT_EQ(init_expander.port_int_cmp_value_reg_image[PORT_A], expected_port_int_cmp_value[PORT_A]);
    EXPECT_EQ(init_expander.port_int_cmp_value_reg_image[PORT_B], expected_port_int_cmp_value[PORT_B]);
    EXPECT_EQ(init_expander.port_int_polarity_reg_image[PORT_A], expected_port_int_polarity[PORT_A]);
    EXPECT_EQ(init_expander.port_int_polarity_reg_image[PORT_B], expected_port_int_polarity[PORT_B]);
    EXPECT_EQ(init_expander.port_int_pullup_reg_image[PORT_A], expected_port_int_pullup[PORT_A]);
    EXPECT_EQ(init_expander.port_int_pullup_reg_image[PORT_B], expected_port_int_pullup[PORT_B]);
}

TEST_F(Mcp23017ExpanderTestFixture, SetPortDirection_1)
{
    // Arrange: create and set up a system under test
    uint8_t expected_port_direction[2];
    expected_port_direction[PORT_A] = 0x00;
    expected_port_direction[PORT_B] = 0xFF;

    ON_CALL(*mockConfigI2C, writeI2C)
            .WillByDefault(testing::Return(STD_SUCCESS));

    // Act: poke the system under test
    mcp23017_expander_set_port_direction(&expander, PORT_A, OUTPUT_DIRECTION, &error);
    mcp23017_expander_set_port_direction(&expander, PORT_B, OUTPUT_DIRECTION, &error);
    mcp23017_expander_set_port_direction(&expander, PORT_B, INPUT_DIRECTION, &error);

    // Assert: make unit test pass or fail
    EXPECT_EQ(expander.port_direction_reg_image[PORT_A], expected_port_direction[PORT_A]);
    EXPECT_EQ(expander.port_direction_reg_image[PORT_B], expected_port_direction[PORT_B]);
}

TEST_F(Mcp23017ExpanderTestFixture, SetPortDirection_2)
{
    // Arrange: create and set up a system under test
    uint8_t expected_port_direction[2];
    expected_port_direction[PORT_A] = 0x00;

    EXPECT_CALL(*mockConfigI2C, writeI2C(MCP23017_DEVICE_ADDRESS, MCP23017_PORT_A_DIRECTION_REGISTER_ADDRESS,
                testing::_, testing::Pointee(0x00), testing::_, testing::_, testing::_))
                .Times(1)
                .WillRepeatedly(testing::Return(STD_SUCCESS));

    // Act: poke the system under test
    mcp23017_expander_set_port_direction(&expander, PORT_A, OUTPUT_DIRECTION, &error);

    // Assert: make unit test pass or fail
    EXPECT_EQ(expander.port_direction_reg_image[PORT_A], expected_port_direction[PORT_A]);
}

TEST_F(Mcp23017ExpanderTestFixture, SetPortDirection_3)
{
    // Arrange: create and set up a system under test
    int expected_ret_val = STD_FAILURE;

    EXPECT_CALL(*mockConfigI2C, writeI2C(MCP23017_DEVICE_ADDRESS, MCP23017_PORT_A_DIRECTION_REGISTER_ADDRESS,
                testing::_, testing::_, testing::_, testing::_, testing::_))
                .Times(1)
                .WillRepeatedly(testing::Return(STD_FAILURE));

    // Act: poke the system under test
    int ret_val = mcp23017_expander_set_port_direction(&expander, PORT_A, OUTPUT_DIRECTION, &error);

    // Assert: make unit test pass or fail
    EXPECT_EQ(ret_val, expected_ret_val);
}

TEST_F(Mcp23017ExpanderTestFixture, SetPinDirection_1)
{
    // Arrange: create and set up a system under test
    uint8_t expected_port_direction[2];
    expected_port_direction[PORT_A] = 0xFF;
    expected_port_direction[PORT_B] = 0xFF;
    expected_port_direction[PORT_B] &= ~((1 << PIN_2) | (1 << PIN_5) | (1 << PIN_6));

    ON_CALL(*mockConfigI2C, writeI2C)
            .WillByDefault(testing::Return(STD_SUCCESS));

    // Act: poke the system under test
    mcp23017_expander_set_pin_direction(&expander, PORT_B, PIN_0, INPUT_DIRECTION, &error);
    mcp23017_expander_set_pin_direction(&expander, PORT_B, PIN_2, OUTPUT_DIRECTION, &error);
    mcp23017_expander_set_pin_direction(&expander, PORT_B, PIN_5, OUTPUT_DIRECTION, &error);
    mcp23017_expander_set_pin_direction(&expander, PORT_B, PIN_6, OUTPUT_DIRECTION, &error);
    mcp23017_expander_set_pin_direction(&expander, PORT_B, PIN_7, OUTPUT_DIRECTION, &error);
    mcp23017_expander_set_pin_direction(&expander, PORT_B, PIN_7, INPUT_DIRECTION, &error);

    // Assert: make unit test pass or fail
    EXPECT_EQ(expander.port_direction_reg_image[PORT_A], expected_port_direction[PORT_A]);
    EXPECT_EQ(expander.port_direction_reg_image[PORT_B], expected_port_direction[PORT_B]);
}

TEST_F(Mcp23017ExpanderTestFixture, SetPinDirection_2)
{
    // Arrange: create and set up a system under test
    uint8_t expected_port_direction[2];
    expected_port_direction[PORT_B] = 0xFF;
    expected_port_direction[PORT_B] &= ~(1 << PIN_5);

    EXPECT_CALL(*mockConfigI2C, writeI2C(MCP23017_DEVICE_ADDRESS, MCP23017_PORT_B_DIRECTION_REGISTER_ADDRESS,
                testing::_, testing::Pointee(expected_port_direction[PORT_B]), testing::_, testing::_, testing::_))
                .Times(1)
                .WillRepeatedly(testing::Return(STD_SUCCESS));

    // Act: poke the system under test
    mcp23017_expander_set_pin_direction(&expander, PORT_B, PIN_5, OUTPUT_DIRECTION, &error);

    // Assert: make unit test pass or fail
    EXPECT_EQ(expander.port_direction_reg_image[PORT_B], expected_port_direction[PORT_B]);
}

TEST_F(Mcp23017ExpanderTestFixture, SetPortOut_1)
{
    // Arrange: create and set up a system under test
    uint8_t expected_port_out[2];
    expected_port_out[PORT_A] = 0x00;
    expected_port_out[PORT_B] = 0xFF;

    ON_CALL(*mockConfigI2C, writeI2C)
            .WillByDefault(testing::Return(STD_SUCCESS));

    // Act: poke the system under test
    mcp23017_expander_set_port_out(&expander, PORT_A, HIGH_GPIO, &error);
    mcp23017_expander_set_port_out(&expander, PORT_A, LOW_GPIO, &error);
    mcp23017_expander_set_port_out(&expander, PORT_B, HIGH_GPIO, &error);

    // Assert: make unit test pass or fail
    EXPECT_EQ(expander.port_out_reg_image[PORT_A], expected_port_out[PORT_A]);
    EXPECT_EQ(expander.port_out_reg_image[PORT_B], expected_port_out[PORT_B]);
}

TEST_F(Mcp23017ExpanderTestFixture, SetPortOut_2)
{
    // Arrange: create and set up a system under test
    uint8_t expected_port_out[2];
    expected_port_out[PORT_B] = 0xFF;

    EXPECT_CALL(*mockConfigI2C, writeI2C(MCP23017_DEVICE_ADDRESS, MCP23017_PORT_B_OUTPUT_LATCH_REGISTER_ADDRESS,
                testing::_, testing::Pointee(expected_port_out[PORT_B]), testing::_, testing::_, testing::_))
                .Times(1)
                .WillRepeatedly(testing::Return(STD_SUCCESS));

    // Act: poke the system under test
    mcp23017_expander_set_port_out(&expander, PORT_B, HIGH_GPIO, &error);

    // Assert: make unit test pass or fail
    EXPECT_EQ(expander.port_out_reg_image[PORT_B], expected_port_out[PORT_B]);
}

TEST_F(Mcp23017ExpanderTestFixture, SetPinOut_1)
{
    // Arrange: create and set up a system under test
    uint8_t expected_port_out[2];
    expected_port_out[PORT_A] = 0x00;
    expected_port_out[PORT_B] = 0x00;
    expected_port_out[PORT_B] |= (1 << PIN_2) | (1 << PIN_3) | (1 << PIN_4) | (1 << PIN_7);

    ON_CALL(*mockConfigI2C, writeI2C)
            .WillByDefault(testing::Return(STD_SUCCESS));

    // Act: poke the system under test
    mcp23017_expander_set_pin_out(&expander, PORT_B, PIN_0, LOW_GPIO, &error);
    mcp23017_expander_set_pin_out(&expander, PORT_B, PIN_2, HIGH_GPIO, &error);
    mcp23017_expander_set_pin_out(&expander, PORT_B, PIN_3, HIGH_GPIO, &error);
    mcp23017_expander_set_pin_out(&expander, PORT_B, PIN_4, HIGH_GPIO, &error);
    mcp23017_expander_set_pin_out(&expander, PORT_B, PIN_7, HIGH_GPIO, &error);
    mcp23017_expander_set_pin_out(&expander, PORT_B, PIN_5, HIGH_GPIO, &error);
    mcp23017_expander_set_pin_out(&expander, PORT_B, PIN_5, LOW_GPIO, &error);

    // Assert: make unit test pass or fail
    EXPECT_EQ(expander.port_out_reg_image[PORT_A], expected_port_out[PORT_A]);
    EXPECT_EQ(expander.port_out_reg_image[PORT_B], expected_port_out[PORT_B]);
}

TEST_F(Mcp23017ExpanderTestFixture, SetPinOut_2)
{
    // Arrange: create and set up a system under test
    uint8_t expected_port_out[2];
    expected_port_out[PORT_B] = 0x00;
    expected_port_out[PORT_B] |= (1 << PIN_7);

    EXPECT_CALL(*mockConfigI2C, writeI2C(MCP23017_DEVICE_ADDRESS, MCP23017_PORT_B_OUTPUT_LATCH_REGISTER_ADDRESS,
                testing::_, testing::Pointee(expected_port_out[PORT_B]), testing::_, testing::_, testing::_))
                .Times(1)
                .WillRepeatedly(testing::Return(STD_SUCCESS));

    // Act: poke the system under test
    mcp23017_expander_set_pin_out(&expander, PORT_B, PIN_7, HIGH_GPIO, &error);

    // Assert: make unit test pass or fail
    EXPECT_EQ(expander.port_out_reg_image[PORT_B], expected_port_out[PORT_B]);
}

TEST_F(Mcp23017ExpanderTestFixture, SetPinInt_1)
{
    // Arrange: create and set up a system under test
    mcp23017_expander_int_config_t config_pin_3;
    config_pin_3.control    = ENABLE_INTERRUPT;
    config_pin_3.cmp_mode   = ENABLE_COMPARISON;
    config_pin_3.cmp_value  = LOW_COMPARISON_VALUE;
    config_pin_3.polarity   = SAME_POLARITY;
    config_pin_3.pullup     = ENABLE_PULL_UP;

    mcp23017_expander_int_config_t config_pin_7;
    config_pin_7.control    = ENABLE_INTERRUPT;
    config_pin_7.cmp_mode   = DISABLE_COMPARISON;
    config_pin_7.cmp_value  = HIGH_COMPARISON_VALUE;
    config_pin_7.polarity   = INVERTED_POLARITY;
    config_pin_7.pullup     = ENABLE_PULL_UP;

    uint8_t expected_port_int_control[2], expected_port_int_cmp_mode[2], expected_port_int_cmp_value[2];
    uint8_t expected_port_int_polarity[2], expected_port_int_pullup[2];
    expected_port_int_control[PORT_A]   = 0x00;
    expected_port_int_control[PORT_B]   = 0x00;
    expected_port_int_control[PORT_B]   |= (1 << PIN_3) | (1 << PIN_7);
    expected_port_int_cmp_mode[PORT_A]  = 0x00;
    expected_port_int_cmp_mode[PORT_B]  = 0x00;
    expected_port_int_cmp_mode[PORT_B]  |= (1 << PIN_3);
    expected_port_int_cmp_value[PORT_A] = 0x00;
    expected_port_int_cmp_value[PORT_B] = 0x00;
    expected_port_int_cmp_value[PORT_B] |= (1 << PIN_7);
    expected_port_int_polarity[PORT_A]  = 0x00;
    expected_port_int_polarity[PORT_B]  = 0x00;
    expected_port_int_polarity[PORT_B]  |= (1 << PIN_7);
    expected_port_int_pullup[PORT_A]    = 0x00;
    expected_port_int_pullup[PORT_B]    = 0x00;
    expected_port_int_pullup[PORT_B]    |= (1 << PIN_3);

    ON_CALL(*mockConfigI2C, writeI2C)
            .WillByDefault(testing::Return(STD_SUCCESS));

    // Act: poke the system under test
    mcp23017_expander_set_pin_int(&expander, PORT_B, PIN_3, &config_pin_3, &error);
    mcp23017_expander_set_pin_int(&expander, PORT_B, PIN_7, &config_pin_7, &error);

    config_pin_7.pullup= DISABLE_PULL_UP;
    mcp23017_expander_set_pin_int(&expander, PORT_B, PIN_7, &config_pin_7, &error);

    // Assert: make unit test pass or fail
    EXPECT_EQ(expander.port_int_control_reg_image[PORT_A],      expected_port_int_control[PORT_A]);
    EXPECT_EQ(expander.port_int_control_reg_image[PORT_B],      expected_port_int_control[PORT_B]);
    EXPECT_EQ(expander.port_int_cmp_mode_reg_image[PORT_A],     expected_port_int_cmp_mode[PORT_A]);
    EXPECT_EQ(expander.port_int_cmp_mode_reg_image[PORT_B],     expected_port_int_cmp_mode[PORT_B]);
    EXPECT_EQ(expander.port_int_cmp_value_reg_image[PORT_A],    expected_port_int_cmp_value[PORT_A]);
    EXPECT_EQ(expander.port_int_cmp_value_reg_image[PORT_B],    expected_port_int_cmp_value[PORT_B]);
    EXPECT_EQ(expander.port_int_polarity_reg_image[PORT_A],     expected_port_int_polarity[PORT_A]);
    EXPECT_EQ(expander.port_int_polarity_reg_image[PORT_B],     expected_port_int_polarity[PORT_B]);
    EXPECT_EQ(expander.port_int_pullup_reg_image[PORT_A],       expected_port_int_pullup[PORT_A]);
    EXPECT_EQ(expander.port_int_pullup_reg_image[PORT_B],       expected_port_int_pullup[PORT_B]);
}

TEST_F(Mcp23017ExpanderTestFixture, SetPinInt_2)
{
    // Arrange: create and set up a system under test
    mcp23017_expander_int_config_t config_pin_3;
    config_pin_3.control    = ENABLE_INTERRUPT;
    config_pin_3.cmp_mode   = ENABLE_COMPARISON;
    config_pin_3.cmp_value  = LOW_COMPARISON_VALUE;
    config_pin_3.polarity   = SAME_POLARITY;
    config_pin_3.pullup     = ENABLE_PULL_UP;

    uint8_t expected_port_int_control[2], expected_port_int_cmp_mode[2], expected_port_int_cmp_value[2];
    uint8_t expected_port_int_polarity[2], expected_port_int_pullup[2];
    expected_port_int_control[PORT_B]   = 0x00;
    expected_port_int_control[PORT_B]   |= (1 << PIN_3);
    expected_port_int_cmp_mode[PORT_B]  = 0x00;
    expected_port_int_cmp_mode[PORT_B]  |= (1 << PIN_3);
    expected_port_int_cmp_value[PORT_B] = 0x00;
    expected_port_int_polarity[PORT_B]  = 0x00;
    expected_port_int_pullup[PORT_B]    = 0x00;
    expected_port_int_pullup[PORT_B]    |= (1 << PIN_3);

    EXPECT_CALL(*mockConfigI2C, writeI2C(MCP23017_DEVICE_ADDRESS, MCP23017_PORT_B_PULLUP_REGISTER_ADDRESS,
                testing::_, testing::Pointee(expected_port_int_pullup[PORT_B]), testing::_, testing::_, testing::_))
                .Times(1)
                .WillRepeatedly(testing::Return(STD_SUCCESS));

    EXPECT_CALL(*mockConfigI2C, writeI2C(MCP23017_DEVICE_ADDRESS, MCP23017_PORT_B_INTERRUPT_CONTROL_REGISTER_ADDRESS,
                testing::_, testing::Pointee(expected_port_int_cmp_mode[PORT_B]), testing::_, testing::_, testing::_))
                .Times(1)
                .WillRepeatedly(testing::Return(STD_SUCCESS));

    EXPECT_CALL(*mockConfigI2C, writeI2C(MCP23017_DEVICE_ADDRESS, MCP23017_PORT_B_DEFAULT_VALUE_REGISTER_ADDRESS,
                testing::_, testing::Pointee(expected_port_int_cmp_value[PORT_B]), testing::_, testing::_, testing::_))
                .Times(1)
                .WillRepeatedly(testing::Return(STD_SUCCESS));

    EXPECT_CALL(*mockConfigI2C, writeI2C(MCP23017_DEVICE_ADDRESS, MCP23017_PORT_B_ENABLE_INTERRUPT_REGISTER_ADDRESS,
                testing::_, testing::Pointee(expected_port_int_control[PORT_B]), testing::_, testing::_, testing::_))
                .Times(1)
                .WillRepeatedly(testing::Return(STD_SUCCESS));

    EXPECT_CALL(*mockConfigI2C, writeI2C(MCP23017_DEVICE_ADDRESS, MCP23017_PORT_B_POLARITY_REGISTER_ADDRESS,
                testing::_, testing::Pointee(expected_port_int_polarity[PORT_B]), testing::_, testing::_, testing::_))
                .Times(1)
                .WillRepeatedly(testing::Return(STD_SUCCESS));

    // Act: poke the system under test
    mcp23017_expander_set_pin_int(&expander, PORT_B, PIN_3, &config_pin_3, &error);

    // Assert: make unit test pass or fail
    EXPECT_EQ(expander.port_int_control_reg_image[PORT_B],      expected_port_int_control[PORT_B]);
    EXPECT_EQ(expander.port_int_cmp_mode_reg_image[PORT_B],     expected_port_int_cmp_mode[PORT_B]);
    EXPECT_EQ(expander.port_int_cmp_value_reg_image[PORT_B],    expected_port_int_cmp_value[PORT_B]);
    EXPECT_EQ(expander.port_int_polarity_reg_image[PORT_B],     expected_port_int_polarity[PORT_B]);
    EXPECT_EQ(expander.port_int_pullup_reg_image[PORT_B],       expected_port_int_pullup[PORT_B]);
}

TEST_F(Mcp23017ExpanderTestFixture, GetPortIn)
{
    // Arrange: create and set up a system under test
    int expected_ret_val = STD_SUCCESS;

    EXPECT_CALL(*mockConfigI2C, readI2C(MCP23017_DEVICE_ADDRESS, MCP23017_PORT_A_GPIO_REGISTER_ADDRESS,
                testing::_, testing::_, testing::_, testing::_, testing::_))
                .Times(1)
                .WillRepeatedly(testing::Return(STD_SUCCESS));

    // Act: poke the system under test
    uint8_t port_in;
    int ret_val = mcp23017_expander_get_port_in(&expander, PORT_A, &port_in, &error);

    // Assert: make unit test pass or fail
    ASSERT_EQ(ret_val, expected_ret_val);
}

TEST_F(Mcp23017ExpanderTestFixture, GetPortIntFlag)
{
    // Arrange: create and set up a system under test
    int expected_ret_val = STD_SUCCESS;

    EXPECT_CALL(*mockConfigI2C, readI2C(MCP23017_DEVICE_ADDRESS, MCP23017_PORT_B_INTERRUPT_FLAG_REGISTER_ADDRESS,
                testing::_, testing::_, testing::_, testing::_, testing::_))
                .Times(1)
                .WillRepeatedly(testing::Return(STD_SUCCESS));

    // Act: poke the system under test
    uint8_t port_flag;
    int ret_val = mcp23017_expander_get_int_flag(&expander, PORT_B, &port_flag, &error);

    // Assert: make unit test pass or fail
    ASSERT_EQ(ret_val, expected_ret_val);
}

TEST_F(Mcp23017ExpanderTestFixture, GetPortIntCapture)
{
    // Arrange: create and set up a system under test
    int expected_ret_val = STD_SUCCESS;

    EXPECT_CALL(*mockConfigI2C, readI2C(MCP23017_DEVICE_ADDRESS, MCP23017_PORT_A_INTERRUPT_CAPTURED_REGISTER_ADDRESS,
                testing::_, testing::_, testing::_, testing::_, testing::_))
                .Times(1)
                .WillRepeatedly(testing::Return(STD_SUCCESS));

    // Act: poke the system under test
    uint8_t port_capture;
    int ret_val = mcp23017_expander_get_int_capture(&expander, PORT_A, &port_capture, &error);

    // Assert: make unit test pass or fail
    ASSERT_EQ(ret_val, expected_ret_val);
}