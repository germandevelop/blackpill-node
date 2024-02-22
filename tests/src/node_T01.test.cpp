/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#include <gmock/gmock.h>

#include "node_T01.h"
#include "std_error/std_error.h"


class NodeT01TestFixture : public ::testing::Test
{
    protected:

        node_T01_t node;

        virtual void SetUp() override
        {
            node_T01_init(&node);
        }
};


TEST_F(NodeT01TestFixture, Init)
{
    // Arrange: create and set up a system under test
    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = false;
    expected_state.status_led_color     = GREEN_COLOR;
    expected_state.is_light_on          = false;
    expected_state.is_display_on        = true;
    expected_state.is_warning_led_on    = false;

    // Act: poke the system under test
    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
}

TEST_F(NodeT01TestFixture, ProcessLuminosity_0)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity;
    luminosity.adc      = NODE_T01_DARKNESS_LEVEL_ADC;
    luminosity.is_valid = false;

    bool expected_is_dark = false;

    // Act: poke the system under test
    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    // Assert: make unit test pass or fail
    EXPECT_EQ(node.is_dark, expected_is_dark);
}

TEST_F(NodeT01TestFixture, ProcessLuminosity_1)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity;
    luminosity.adc      = NODE_T01_DARKNESS_LEVEL_ADC;
    luminosity.is_valid = true;

    bool expected_is_dark = false;

    // Act: poke the system under test
    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    // Assert: make unit test pass or fail
    EXPECT_EQ(node.is_dark, expected_is_dark);
}

TEST_F(NodeT01TestFixture, ProcessLuminosity_2)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity;
    luminosity.adc      = NODE_T01_DARKNESS_LEVEL_ADC + 1U;
    luminosity.is_valid = true;

    bool expected_is_dark = true;

    // Act: poke the system under test
    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    // Assert: make unit test pass or fail
    EXPECT_EQ(node.is_dark, expected_is_dark);
}

TEST_F(NodeT01TestFixture, ProcessReedSwitch_0)
{
    // Arrange: create and set up a system under test
    bool is_reed_switch_open = false;

    bool expected_is_door_open      = false;
    bool expected_is_msg_to_send    = true;
    node_command_id_t expected_cmd  = UPDATE_DOOR_STATE;

    // Act: poke the system under test
    uint32_t next_time_ms;
    node_T01_process_reed_switch(&node, is_reed_switch_open, &next_time_ms);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    // Assert: make unit test pass or fail
    EXPECT_EQ(node.is_door_open,                expected_is_door_open);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_is_msg_to_send);
    EXPECT_EQ(node.send_msg_buffer[0].cmd_id,   expected_cmd);
}

TEST_F(NodeT01TestFixture, ProcessReedSwitch_1)
{
    // Arrange: create and set up a system under test
    bool is_reed_switch_open = true;

    bool expected_is_door_open      = true;
    bool expected_is_msg_to_send    = true;
    node_command_id_t expected_cmd  = UPDATE_DOOR_STATE;

    // Act: poke the system under test
    uint32_t next_time_ms;
    node_T01_process_reed_switch(&node, is_reed_switch_open, &next_time_ms);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    // Assert: make unit test pass or fail
    EXPECT_EQ(node.is_door_open,                expected_is_door_open);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_is_msg_to_send);
    EXPECT_EQ(node.send_msg_buffer[0].cmd_id,   expected_cmd);
}

TEST_F(NodeT01TestFixture, ProcessHumidity_0)
{
    // Arrange: create and set up a system under test
    node_T01_humidity_t humidity;
    humidity.is_valid = false;

    bool expected_is_msg_to_send    = false;
    bool expected_is_valid          = false;

    // Act: poke the system under test
    uint32_t next_time_ms;
    node_T01_process_humidity(&node, &humidity, &next_time_ms);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,  expected_is_msg_to_send);
    EXPECT_EQ(node.humidity.is_valid,       expected_is_valid);
}

TEST_F(NodeT01TestFixture, ProcessHumidity_1)
{
    // Arrange: create and set up a system under test
    node_T01_humidity_t humidity;
    humidity.temperature_C  = (-12.5F);
    humidity.is_valid       = true;

    bool expected_is_msg_to_send    = true;
    bool expected_is_valid          = true;
    node_command_id_t expected_cmd  = UPDATE_TEMPERATURE;
    float expected_value_2          = (-12.5F);

    // Act: poke the system under test
    uint32_t next_time_ms;
    node_T01_process_humidity(&node, &humidity, &next_time_ms);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,              expected_is_msg_to_send);
    EXPECT_EQ(node.humidity.is_valid,                   expected_is_valid);
    EXPECT_EQ(node.send_msg_buffer[0].cmd_id,           expected_cmd);
    EXPECT_FLOAT_EQ(node.send_msg_buffer[0].value_2,    expected_value_2);
}

TEST_F(NodeT01TestFixture, ProcessReedSwitchAndHumidity_0)
{
    // Arrange: create and set up a system under test
    bool is_reed_switch_open = true;

    node_T01_humidity_t humidity;
    humidity.temperature_C  = (NODE_T01_LOW_TEMPERATURE_C - 1.0F);
    humidity.is_valid       = true;

    size_t expected_msg_count   = 2U;
    bool expected_is_door_open  = true;

    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = true;
    expected_state.status_led_color     = GREEN_COLOR;
    expected_state.is_light_on          = false;
    expected_state.is_display_on        = false;
    expected_state.is_warning_led_on    = true;

    // Act: poke the system under test
    uint32_t next_time_ms;
    node_T01_process_reed_switch(&node, is_reed_switch_open, &next_time_ms);
    node_T01_process_humidity(&node, &humidity, &next_time_ms);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
    EXPECT_EQ(node.is_door_open,                expected_is_door_open);
    EXPECT_EQ(node.send_msg_buffer_size,        expected_msg_count);
}

TEST_F(NodeT01TestFixture, ProcessReedSwitchAndHumidity_1)
{
    // Arrange: create and set up a system under test
    bool is_reed_switch_open = true;

    node_T01_humidity_t humidity;
    humidity.temperature_C  = (NODE_T01_LOW_TEMPERATURE_C + 1.0F);
    humidity.is_valid       = true;

    size_t expected_msg_count   = 2U;
    bool expected_is_door_open  = true;

    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = true;
    expected_state.status_led_color     = GREEN_COLOR;
    expected_state.is_light_on          = false;
    expected_state.is_display_on        = false;
    expected_state.is_warning_led_on    = false;

    // Act: poke the system under test
    uint32_t next_time_ms;
    node_T01_process_reed_switch(&node, is_reed_switch_open, &next_time_ms);
    node_T01_process_humidity(&node, &humidity, &next_time_ms);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
    EXPECT_EQ(node.is_door_open,                expected_is_door_open);
    EXPECT_EQ(node.send_msg_buffer_size,        expected_msg_count);
}

TEST_F(NodeT01TestFixture, ProcessReedSwitchAndHumidity_2)
{
    // Arrange: create and set up a system under test
    bool is_reed_switch_open = false;

    node_T01_humidity_t humidity;
    humidity.temperature_C  = (NODE_T01_HIGH_TEMPERATURE_C - 1.0F);
    humidity.is_valid       = true;

    size_t expected_msg_count   = 2U;
    bool expected_is_door_open  = false;

    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = true;
    expected_state.status_led_color     = GREEN_COLOR;
    expected_state.is_light_on          = false;
    expected_state.is_display_on        = false;
    expected_state.is_warning_led_on    = false;

    // Act: poke the system under test
    uint32_t next_time_ms;
    node_T01_process_reed_switch(&node, is_reed_switch_open, &next_time_ms);
    node_T01_process_humidity(&node, &humidity, &next_time_ms);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
    EXPECT_EQ(node.is_door_open,                expected_is_door_open);
    EXPECT_EQ(node.send_msg_buffer_size,        expected_msg_count);
}

TEST_F(NodeT01TestFixture, ProcessReedSwitchAndHumidity_3)
{
    // Arrange: create and set up a system under test
    bool is_reed_switch_open = false;

    node_T01_humidity_t humidity;
    humidity.temperature_C  = (NODE_T01_HIGH_TEMPERATURE_C + 1.0F);
    humidity.is_valid       = true;

    size_t expected_msg_count   = 2U;
    bool expected_is_door_open  = false;

    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = true;
    expected_state.status_led_color     = GREEN_COLOR;
    expected_state.is_light_on          = false;
    expected_state.is_display_on        = false;
    expected_state.is_warning_led_on    = true;

    // Act: poke the system under test
    uint32_t next_time_ms;
    node_T01_process_reed_switch(&node, is_reed_switch_open, &next_time_ms);
    node_T01_process_humidity(&node, &humidity, &next_time_ms);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
    EXPECT_EQ(node.is_door_open,                expected_is_door_open);
    EXPECT_EQ(node.send_msg_buffer_size,        expected_msg_count);
}

TEST_F(NodeT01TestFixture, ProcessMsgMode_0)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity;
    luminosity.adc      = NODE_T01_DARKNESS_LEVEL_ADC;
    luminosity.is_valid = true;

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    node_msg_t rcv_msg;
    rcv_msg.cmd_id  = SET_MODE;
    rcv_msg.value_0 = (uint32_t)(ALARM);

    node_mode_id_t expected_mode = ALARM;
    bool expected_is_dark = false;

    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = false;
    expected_state.status_led_color     = RED_COLOR;
    expected_state.is_light_on          = false;
    expected_state.is_display_on        = false;
    expected_state.is_warning_led_on    = true;

    // Act: poke the system under test
    node_T01_process_rcv_msg(&node, &rcv_msg, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS + 1U);

    // Assert: make unit test pass or fail
    ASSERT_EQ(node.mode,                        expected_mode);
    ASSERT_EQ(node.is_dark,                     expected_is_dark);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
}

TEST_F(NodeT01TestFixture, ProcessMsgMode_1)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity;
    luminosity.adc      = NODE_T01_DARKNESS_LEVEL_ADC + 1U;
    luminosity.is_valid = true;

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    node_msg_t rcv_msg;
    rcv_msg.cmd_id  = SET_MODE;
    rcv_msg.value_0 = (uint32_t)(ALARM);

    node_mode_id_t expected_mode = ALARM;
    bool expected_is_dark = true;

    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = false;
    expected_state.status_led_color     = RED_COLOR;
    expected_state.is_light_on          = true;
    expected_state.is_display_on        = false;
    expected_state.is_warning_led_on    = true;

    // Act: poke the system under test
    node_T01_process_rcv_msg(&node, &rcv_msg, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS + 1U);

    // Assert: make unit test pass or fail
    ASSERT_EQ(node.mode,                        expected_mode);
    ASSERT_EQ(node.is_dark,                     expected_is_dark);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
}

TEST_F(NodeT01TestFixture, ProcessMsgMode_2)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity;
    luminosity.adc      = NODE_T01_DARKNESS_LEVEL_ADC;
    luminosity.is_valid = true;

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    node_msg_t rcv_msg;
    rcv_msg.cmd_id  = SET_MODE;
    rcv_msg.value_0 = (uint32_t)(GUARD);

    node_mode_id_t expected_mode = GUARD;
    bool expected_is_dark = false;

    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = false;
    expected_state.status_led_color     = RED_COLOR;
    expected_state.is_light_on          = false;
    expected_state.is_display_on        = false;
    expected_state.is_warning_led_on    = false;

    // Act: poke the system under test
    node_T01_process_rcv_msg(&node, &rcv_msg, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    // Assert: make unit test pass or fail
    ASSERT_EQ(node.mode,                        expected_mode);
    ASSERT_EQ(node.is_dark,                     expected_is_dark);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
}

TEST_F(NodeT01TestFixture, ProcessMsgMode_3)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity;
    luminosity.adc      = NODE_T01_DARKNESS_LEVEL_ADC + 1U;
    luminosity.is_valid = true;

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    node_msg_t rcv_msg;
    rcv_msg.cmd_id  = SET_MODE;
    rcv_msg.value_0 = (uint32_t)(GUARD);

    node_mode_id_t expected_mode = GUARD;
    bool expected_is_dark = true;

    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = false;
    expected_state.status_led_color     = RED_COLOR;
    expected_state.is_light_on          = true;
    expected_state.is_display_on        = false;
    expected_state.is_warning_led_on    = false;

    // Act: poke the system under test
    node_T01_process_rcv_msg(&node, &rcv_msg, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    // Assert: make unit test pass or fail
    ASSERT_EQ(node.mode,                        expected_mode);
    ASSERT_EQ(node.is_dark,                     expected_is_dark);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
}

TEST_F(NodeT01TestFixture, ProcessMsgLight_0)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity;
    luminosity.adc      = NODE_T01_DARKNESS_LEVEL_ADC;
    luminosity.is_valid = true;

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    node_msg_t rcv_msg;
    rcv_msg.cmd_id  = SET_LIGHT;
    rcv_msg.value_0 = (uint32_t)(LIGHT_ON);

    node_mode_id_t expected_mode = SILENCE;
    bool expected_is_dark = false;

    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = false;
    expected_state.status_led_color     = GREEN_COLOR;
    expected_state.is_light_on          = false;
    expected_state.is_display_on        = false;
    expected_state.is_warning_led_on    = false;

    // Act: poke the system under test
    node_T01_process_rcv_msg(&node, &rcv_msg, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS + 1U);

    // Assert: make unit test pass or fail
    ASSERT_EQ(node.mode,                        expected_mode);
    ASSERT_EQ(node.is_dark,                     expected_is_dark);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
}

TEST_F(NodeT01TestFixture, ProcessMsgLight_1)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity;
    luminosity.adc      = NODE_T01_DARKNESS_LEVEL_ADC + 1U;
    luminosity.is_valid = true;

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    node_msg_t rcv_msg;
    rcv_msg.cmd_id  = SET_LIGHT;
    rcv_msg.value_0 = (uint32_t)(LIGHT_ON);

    node_mode_id_t expected_mode = SILENCE;
    bool expected_is_dark = true;

    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = false;
    expected_state.status_led_color     = GREEN_COLOR;
    expected_state.is_light_on          = true;
    expected_state.is_display_on        = true;
    expected_state.is_warning_led_on    = false;

    // Act: poke the system under test
    node_T01_process_rcv_msg(&node, &rcv_msg, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS + 1U);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS + 2U);

    // Assert: make unit test pass or fail
    ASSERT_EQ(node.mode,                        expected_mode);
    ASSERT_EQ(node.is_dark,                     expected_is_dark);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
}

TEST_F(NodeT01TestFixture, ProcessMsgLight_2)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity;
    luminosity.adc      = NODE_T01_DARKNESS_LEVEL_ADC + 1U;
    luminosity.is_valid = true;

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    node_msg_t rcv_msg;
    rcv_msg.cmd_id  = SET_LIGHT;
    rcv_msg.value_0 = (uint32_t)(LIGHT_ON);

    node_mode_id_t expected_mode = SILENCE;
    bool expected_is_dark = true;

    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = false;
    expected_state.status_led_color     = GREEN_COLOR;
    expected_state.is_light_on          = false;
    expected_state.is_display_on        = false;
    expected_state.is_warning_led_on    = false;

    // Act: poke the system under test
    node_T01_process_rcv_msg(&node, &rcv_msg, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS + 1U);

    // Assert: make unit test pass or fail
    ASSERT_EQ(node.mode,                        expected_mode);
    ASSERT_EQ(node.is_dark,                     expected_is_dark);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
}

TEST_F(NodeT01TestFixture, ProcessMsgLight_3)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity;
    luminosity.adc      = NODE_T01_DARKNESS_LEVEL_ADC + 1U;
    luminosity.is_valid = true;

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    node_msg_t rcv_msg;
    rcv_msg.cmd_id  = SET_LIGHT;
    rcv_msg.value_0 = (uint32_t)(LIGHT_ON);

    node_T01_process_rcv_msg(&node, &rcv_msg, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS + 1U);

    node_mode_id_t expected_mode = SILENCE;
    bool expected_is_dark = true;

    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = false;
    expected_state.status_led_color     = GREEN_COLOR;
    expected_state.is_light_on          = false;
    expected_state.is_display_on        = false;
    expected_state.is_warning_led_on    = false;

    // Act: poke the system under test
    rcv_msg.value_0 = (uint32_t)(LIGHT_OFF);
    node_T01_process_rcv_msg(&node, &rcv_msg, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS + 2U);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS + 3U);

    // Assert: make unit test pass or fail
    ASSERT_EQ(node.mode,                        expected_mode);
    ASSERT_EQ(node.is_dark,                     expected_is_dark);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
}

TEST_F(NodeT01TestFixture, ProcessFrontPir_0)
{
    // Arrange: create and set up a system under test
    node_msg_t rcv_msg;
    rcv_msg.cmd_id  = SET_MODE;
    rcv_msg.value_0 = (uint32_t)(GUARD);

    node_T01_process_rcv_msg(&node, &rcv_msg, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    node_T01_luminosity_t luminosity;
    luminosity.adc      = NODE_T01_DARKNESS_LEVEL_ADC + 1U;
    luminosity.is_valid = true;

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    node_mode_id_t expected_mode = GUARD;
    bool expected_is_dark = true;

    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = false;
    expected_state.status_led_color     = RED_COLOR;
    expected_state.is_light_on          = false;
    expected_state.is_display_on        = false;
    expected_state.is_warning_led_on    = false;

    // Act: poke the system under test
    node_T01_process_front_pir(&node, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS + 1U);

    // Assert: make unit test pass or fail
    ASSERT_EQ(node.mode,                        expected_mode);
    ASSERT_EQ(node.is_dark,                     expected_is_dark);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
}

TEST_F(NodeT01TestFixture, ProcessFrontPir_1)
{
    // Arrange: create and set up a system under test
    node_msg_t rcv_msg;
    rcv_msg.cmd_id  = SET_MODE;
    rcv_msg.value_0 = (uint32_t)(GUARD);

    node_T01_process_rcv_msg(&node, &rcv_msg, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    node_T01_luminosity_t luminosity;
    luminosity.adc      = NODE_T01_DARKNESS_LEVEL_ADC + 1U;
    luminosity.is_valid = true;

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    node_mode_id_t expected_mode = GUARD;
    bool expected_is_dark = true;

    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = true;
    expected_state.status_led_color     = RED_COLOR;
    expected_state.is_light_on          = true;
    expected_state.is_display_on        = false;
    expected_state.is_warning_led_on    = false;

    node_command_id_t expected_cmd  = SET_INTRUSION;
    uint32_t expected_value_0       = (uint32_t)(INTRUSION_ON);
    

    // Act: poke the system under test
    node_T01_process_front_pir(&node, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS + 1U);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS + 2U);

    // Assert: make unit test pass or fail
    ASSERT_EQ(node.mode,                                expected_mode);
    ASSERT_EQ(node.is_dark,                             expected_is_dark);
    EXPECT_EQ(result_state.is_msg_to_send,              expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,            expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,                 expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,               expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,           expected_state.is_warning_led_on);
    EXPECT_EQ(node.send_msg_buffer[0].cmd_id,           expected_cmd);
    EXPECT_FLOAT_EQ(node.send_msg_buffer[0].value_0,    expected_value_0);
}

TEST_F(NodeT01TestFixture, ProcessFrontPir_2)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity;
    luminosity.adc      = NODE_T01_DARKNESS_LEVEL_ADC;
    luminosity.is_valid = true;

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    node_mode_id_t expected_mode = SILENCE;
    bool expected_is_dark = false;

    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = false;
    expected_state.status_led_color     = GREEN_COLOR;
    expected_state.is_light_on          = false;
    expected_state.is_display_on        = true;
    expected_state.is_warning_led_on    = false;

    // Act: poke the system under test
    node_T01_process_front_pir(&node, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS + 1U);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS + 2U);

    // Assert: make unit test pass or fail
    ASSERT_EQ(node.mode,                        expected_mode);
    ASSERT_EQ(node.is_dark,                     expected_is_dark);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
}

TEST_F(NodeT01TestFixture, ProcessFrontPir_3)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity;
    luminosity.adc      = NODE_T01_DARKNESS_LEVEL_ADC + 1U;
    luminosity.is_valid = true;

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    node_mode_id_t expected_mode = SILENCE;
    bool expected_is_dark = true;

    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = true;
    expected_state.status_led_color     = GREEN_COLOR;
    expected_state.is_light_on          = true;
    expected_state.is_display_on        = true;
    expected_state.is_warning_led_on    = false;

    node_command_id_t expected_cmd  = SET_LIGHT;
    uint32_t expected_value_0       = (uint32_t)(LIGHT_ON);

    // Act: poke the system under test
    node_T01_process_front_pir(&node, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS + 1U);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS + 2U);

    // Assert: make unit test pass or fail
    ASSERT_EQ(node.mode,                                expected_mode);
    ASSERT_EQ(node.is_dark,                             expected_is_dark);
    EXPECT_EQ(result_state.is_msg_to_send,              expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,            expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,                 expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,               expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,           expected_state.is_warning_led_on);
    EXPECT_EQ(node.send_msg_buffer[0].cmd_id,           expected_cmd);
    EXPECT_FLOAT_EQ(node.send_msg_buffer[0].value_0,    expected_value_0);
}

TEST_F(NodeT01TestFixture, ProcessFrontPir_4)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity;
    luminosity.adc      = NODE_T01_DARKNESS_LEVEL_ADC + 1U;
    luminosity.is_valid = true;

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    node_mode_id_t expected_mode = SILENCE;
    bool expected_is_dark = true;

    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = false;
    expected_state.status_led_color     = GREEN_COLOR;
    expected_state.is_light_on          = false;
    expected_state.is_display_on        = false;
    expected_state.is_warning_led_on    = false;

    // Act: poke the system under test
    node_T01_process_front_pir(&node, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS + 1U);

    // Assert: make unit test pass or fail
    ASSERT_EQ(node.mode,                        expected_mode);
    ASSERT_EQ(node.is_dark,                     expected_is_dark);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
}
