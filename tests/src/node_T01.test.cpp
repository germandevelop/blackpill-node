/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#include <gmock/gmock.h>

#include "node_T01.h"
#include "std_error/std_error.h"


class NodeT01TestFixture : public testing::Test
{
    protected:

        node_T01_t node;

        virtual void SetUp() override
        {
            node_T01_init(&node);
        }
};


TEST_F(NodeT01TestFixture, InitZero)
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
    node_T01_get_state(&node, &result_state, 0U);

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
}

TEST_F(NodeT01TestFixture, InitStable)
{
    // Arrange: create and set up a system under test
    node_T01_state_t expected_state;
    expected_state.is_msg_to_send       = false;
    expected_state.status_led_color     = GREEN_COLOR;
    expected_state.is_light_on          = false;
    expected_state.is_display_on        = false;
    expected_state.is_warning_led_on    = false;

    // Act: poke the system under test
    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, (NODE_T01_LIGHT_DURATION_MS + 1U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
}


class NodeT01ParameterizedLuminosity : public NodeT01TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_T01_luminosity_t,
        bool
    >>
{};

TEST_P(NodeT01ParameterizedLuminosity, ProcessLuminosity)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity = std::get<0>(GetParam());

    bool expected_is_dark = std::get<1>(GetParam());

    // Act: poke the system under test
    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    // Assert: make unit test pass or fail
    EXPECT_EQ(node.is_dark, expected_is_dark);
}

INSTANTIATE_TEST_SUITE_P(NodeT01TestFixture, NodeT01ParameterizedLuminosity,
    testing::Values
    (
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX),       .is_valid = false },    false),
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F),.is_valid = false },    false),
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX),       .is_valid = true },     false),
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F),.is_valid = true },     true)
    )
);


class NodeT01ParameterizedDoor : public NodeT01TestFixture, public testing::WithParamInterface
    <std::tuple<
        bool,
        node_T01_state_t
    >>
{};

TEST_P(NodeT01ParameterizedDoor, ProcessDoorState)
{
    // Arrange: create and set up a system under test
    bool is_door_open = std::get<0>(GetParam());

    bool expected_is_door_open      = is_door_open;
    node_T01_state_t expected_state = std::get<1>(GetParam());

    // Act: poke the system under test
    uint32_t next_time_ms;
    node_T01_process_door_state(&node, is_door_open, &next_time_ms);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, (NODE_T01_LIGHT_DURATION_MS * 2U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(node.is_door_open,            expected_is_door_open);
    EXPECT_EQ(result_state.is_msg_to_send,  expected_state.is_msg_to_send);
}

INSTANTIATE_TEST_SUITE_P(NodeT01TestFixture, NodeT01ParameterizedDoor,
    testing::Values
    (
        std::make_tuple(false,  node_T01_state_t { .is_msg_to_send = true }),
        std::make_tuple(true,   node_T01_state_t { .is_msg_to_send = true })
    )
);


class NodeT01ParameterizedHumidity : public NodeT01TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_T01_humidity_t,
        node_T01_state_t
    >>
{};

TEST_P(NodeT01ParameterizedHumidity, ProcessHumidity)
{
    // Arrange: create and set up a system under test
    node_T01_humidity_t humidity = std::get<0>(GetParam());

    node_T01_humidity_t expected_humidity   = humidity;
    node_T01_state_t expected_state         = std::get<1>(GetParam());

    // Act: poke the system under test
    uint32_t next_time_ms;
    node_T01_process_humidity(&node, &humidity, &next_time_ms);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, (NODE_T01_LIGHT_DURATION_MS * 2U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(node.humidity.is_valid,       expected_humidity.is_valid);
    EXPECT_EQ(result_state.is_msg_to_send,  expected_state.is_msg_to_send);
}

INSTANTIATE_TEST_SUITE_P(NodeT01TestFixture, NodeT01ParameterizedHumidity,
    testing::Values
    (
        std::make_tuple(node_T01_humidity_t { .is_valid = false },  node_T01_state_t { .is_msg_to_send = false }),
        std::make_tuple(node_T01_humidity_t { .is_valid = true },   node_T01_state_t { .is_msg_to_send = true })
    )
);


class NodeT01ParameterizedDoorAndHumidity : public NodeT01TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_msg_t,
        bool,
        node_T01_humidity_t,
        node_T01_state_t,
        size_t
    >>
{};

TEST_P(NodeT01ParameterizedDoorAndHumidity, ProcessDoorAndHumidity)
{
    // Arrange: create and set up a system under test
    node_msg_t rcv_msg              = std::get<0>(GetParam());
    bool is_door_open               = std::get<1>(GetParam());
    node_T01_humidity_t humidity    = std::get<2>(GetParam());

    node_T01_process_msg(&node, &rcv_msg, (NODE_T01_LIGHT_DURATION_MS * 2U));

    bool expected_is_door_open      = is_door_open;
    node_T01_state_t expected_state = std::get<3>(GetParam());
    size_t expected_msg_count       = std::get<4>(GetParam());

    // Act: poke the system under test
    uint32_t next_time_ms;
    node_T01_process_door_state(&node, is_door_open, &next_time_ms);
    node_T01_process_humidity(&node, &humidity, &next_time_ms);

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, ((NODE_T01_LIGHT_DURATION_MS * 2U) + 1U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(node.is_door_open,                expected_is_door_open);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
    EXPECT_EQ(node.send_msg_buffer_size,        expected_msg_count);
}

INSTANTIATE_TEST_SUITE_P(NodeT01TestFixture, NodeT01ParameterizedDoorAndHumidity,
    testing::Values
    (
        // Warnings enabled
        std::make_tuple(node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                        .cmd_id = SET_WARNING, .value_0 = (int32_t)(WARNING_ON) },
                        true, node_T01_humidity_t { .is_valid = false },
                        node_T01_state_t { .is_warning_led_on = false, .is_msg_to_send = true }, 1U),

        std::make_tuple(node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                        .cmd_id = SET_WARNING, .value_0 = (int32_t)(WARNING_ON) },
                        false, node_T01_humidity_t { .is_valid = false },
                        node_T01_state_t { .is_warning_led_on = false, .is_msg_to_send = true }, 1U),

        std::make_tuple(node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                        .cmd_id = SET_WARNING, .value_0 = (int32_t)(WARNING_ON) },
                        true, node_T01_humidity_t { .temperature_C = (NODE_T01_LOW_TEMPERATURE_C - 1.0F), .is_valid = true },
                        node_T01_state_t { .is_warning_led_on = true, .is_msg_to_send = true }, 2U),

        std::make_tuple(node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                        .cmd_id = SET_WARNING, .value_0 = (int32_t)(WARNING_ON) },
                        true, node_T01_humidity_t { .temperature_C = (NODE_T01_LOW_TEMPERATURE_C + 1.0F), .is_valid = true },
                        node_T01_state_t { .is_warning_led_on = false, .is_msg_to_send = true }, 2U),

        std::make_tuple(node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                        .cmd_id = SET_WARNING, .value_0 = (int32_t)(WARNING_ON) },
                        false, node_T01_humidity_t { .temperature_C = (NODE_T01_HIGH_TEMPERATURE_C - 1.0F), .is_valid = true },
                        node_T01_state_t { .is_warning_led_on = false, .is_msg_to_send = true }, 2U),

        std::make_tuple(node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                        .cmd_id = SET_WARNING, .value_0 = (int32_t)(WARNING_ON) },
                        false, node_T01_humidity_t { .temperature_C = (NODE_T01_HIGH_TEMPERATURE_C + 1.0F), .is_valid = true },
                        node_T01_state_t { .is_warning_led_on = true, .is_msg_to_send = true }, 2U),

        // Warnings disabled
        std::make_tuple(node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                        .cmd_id = SET_WARNING, .value_0 = (int32_t)(WARNING_OFF) },
                        true, node_T01_humidity_t { .is_valid = false },
                        node_T01_state_t { .is_warning_led_on = false, .is_msg_to_send = true }, 1U),

        std::make_tuple(node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                        .cmd_id = SET_WARNING, .value_0 = (int32_t)(WARNING_OFF) },
                        false, node_T01_humidity_t { .is_valid = false },
                        node_T01_state_t { .is_warning_led_on = false, .is_msg_to_send = true }, 1U),

        std::make_tuple(node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                        .cmd_id = SET_WARNING, .value_0 = (int32_t)(WARNING_OFF) },
                        true, node_T01_humidity_t { .temperature_C = (NODE_T01_LOW_TEMPERATURE_C - 1.0F), .is_valid = true },
                        node_T01_state_t { .is_warning_led_on = false, .is_msg_to_send = true }, 2U),

        std::make_tuple(node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                        .cmd_id = SET_WARNING, .value_0 = (int32_t)(WARNING_OFF) },
                        true, node_T01_humidity_t { .temperature_C = (NODE_T01_LOW_TEMPERATURE_C + 1.0F), .is_valid = true },
                        node_T01_state_t { .is_warning_led_on = false, .is_msg_to_send = true }, 2U),

        std::make_tuple(node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                        .cmd_id = SET_WARNING, .value_0 = (int32_t)(WARNING_OFF) },
                        false, node_T01_humidity_t { .temperature_C = (NODE_T01_HIGH_TEMPERATURE_C - 1.0F), .is_valid = true },
                        node_T01_state_t { .is_warning_led_on = false, .is_msg_to_send = true }, 2U),

        std::make_tuple(node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                        .cmd_id = SET_WARNING, .value_0 = (int32_t)(WARNING_OFF) },
                        false, node_T01_humidity_t { .temperature_C = (NODE_T01_HIGH_TEMPERATURE_C + 1.0F), .is_valid = true },
                        node_T01_state_t { .is_warning_led_on = false, .is_msg_to_send = true }, 2U)
    )
);


class NodeT01ParameterizedMsgMode : public NodeT01TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_T01_luminosity_t,
        node_msg_t,
        node_T01_state_t
    >>
{};

TEST_P(NodeT01ParameterizedMsgMode, ProcessMsgMode)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity    = std::get<0>(GetParam());
    node_msg_t rcv_msg                  = std::get<1>(GetParam());

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);

    node_T01_state_t expected_state = std::get<2>(GetParam());

    // Act: poke the system under test
    node_T01_process_msg(&node, &rcv_msg, (NODE_T01_LIGHT_DURATION_MS * 2U));

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, ((NODE_T01_LIGHT_DURATION_MS * 2U) + 1U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
}

INSTANTIATE_TEST_SUITE_P(NodeT01TestFixture, NodeT01ParameterizedMsgMode,
    testing::Values
    (
        // Alarm mode
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        // Guard mode
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        // Silence mode
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false })
    )
);



class NodeT01ParameterizedMsgCommand : public NodeT01TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_T01_luminosity_t,
        node_msg_t,
        node_msg_t,
        node_T01_state_t
    >>
{};

TEST_P(NodeT01ParameterizedMsgCommand, ProcessMsgCommand)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity    = std::get<0>(GetParam());
    node_msg_t mode_rcv_msg             = std::get<1>(GetParam());
    node_msg_t cmd_rcv_msg              = std::get<2>(GetParam());

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);
    node_T01_process_msg(&node, &mode_rcv_msg, (NODE_T01_LIGHT_DURATION_MS * 2U));

    node_T01_state_t expected_state = std::get<3>(GetParam());

    // Act: poke the system under test
    node_T01_process_msg(&node, &cmd_rcv_msg, ((NODE_T01_LIGHT_DURATION_MS * 2U) + 1U));

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, ((NODE_T01_LIGHT_DURATION_MS * 2U) + 2U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
}

INSTANTIATE_TEST_SUITE_P(NodeT01TestFixture, NodeT01ParameterizedMsgCommand,
    testing::Values
    (
        // Alarm mode
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        // Guard mode (8)
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        // Silence mode (16)
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false })
    )
);



class NodeT01ParameterizedMsgCommandDouble : public NodeT01TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_T01_luminosity_t,
        node_msg_t,
        node_msg_t,
        node_msg_t,
        node_T01_state_t
    >>
{};

TEST_P(NodeT01ParameterizedMsgCommandDouble, ProcessMsgCommandDouble)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity    = std::get<0>(GetParam());
    node_msg_t mode_rcv_msg             = std::get<1>(GetParam());
    node_msg_t cmd_rcv_msg_1            = std::get<2>(GetParam());
    node_msg_t cmd_rcv_msg_2            = std::get<3>(GetParam());

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);
    node_T01_process_msg(&node, &mode_rcv_msg, (NODE_T01_LIGHT_DURATION_MS * 2U));

    node_T01_state_t expected_state = std::get<4>(GetParam());

    // Act: poke the system under test
    node_T01_process_msg(&node, &cmd_rcv_msg_1, ((NODE_T01_LIGHT_DURATION_MS * 2U) + 1U));
    node_T01_process_msg(&node, &cmd_rcv_msg_2, ((NODE_T01_LIGHT_DURATION_MS * 2U) + 2U));

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, ((NODE_T01_LIGHT_DURATION_MS * 2U) + 3U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
}

INSTANTIATE_TEST_SUITE_P(NodeT01TestFixture, NodeT01ParameterizedMsgCommandDouble,
    testing::Values
    (
        // Silence mode
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        // Guard mode
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false })
    )
);



TEST_F(NodeT01TestFixture, NodeT01FrontPir)
{
    // Arrange: create and set up a system under test
    node_T01_state_t expected_state;
    expected_state.status_led_color     = GREEN_COLOR;
    expected_state.is_light_on          = false;
    expected_state.is_display_on        = false;
    expected_state.is_warning_led_on    = false;
    expected_state.is_msg_to_send       = false;

    // Act: poke the system under test
    node_T01_process_movement(&node, (NODE_T01_LIGHT_DURATION_MS * 2U));
    node_T01_process_movement(&node, (NODE_T01_LIGHT_DURATION_MS * 3U));

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, ((NODE_T01_LIGHT_DURATION_MS * 3U) + 1U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
}


class NodeT01ParameterizedFrontPirMode : public NodeT01TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_T01_luminosity_t,
        node_msg_t,
        node_T01_state_t
    >>
{};

TEST_P(NodeT01ParameterizedFrontPirMode, ProcessFrontPirMode)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity    = std::get<0>(GetParam());
    node_msg_t rcv_msg                  = std::get<1>(GetParam());

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);
    node_T01_process_msg(&node, &rcv_msg, (NODE_T01_LIGHT_DURATION_MS * 2U));

    node_T01_state_t expected_state = std::get<2>(GetParam());

    // Act: poke the system under test
    node_T01_process_movement(&node, ((NODE_T01_LIGHT_DURATION_MS * 2U) + 1U));

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, ((NODE_T01_LIGHT_DURATION_MS * 2U) + 2U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
}

INSTANTIATE_TEST_SUITE_P(NodeT01TestFixture, NodeT01ParameterizedFrontPirMode,
    testing::Values
    (
        // Alarm mode
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        // Guard mode
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = true }),

        // Silence mode
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = true,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = true })
    )
);



class NodeT01ParameterizedFrontPirCommandBefore : public NodeT01TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_T01_luminosity_t,
        node_msg_t,
        node_msg_t,
        node_T01_state_t
    >>
{};

TEST_P(NodeT01ParameterizedFrontPirCommandBefore, ProcessFrontPirCommandBefore)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity    = std::get<0>(GetParam());
    node_msg_t mode_rcv_msg             = std::get<1>(GetParam());
    node_msg_t cmd_rcv_msg              = std::get<2>(GetParam());

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);
    node_T01_process_msg(&node, &mode_rcv_msg, (NODE_T01_LIGHT_DURATION_MS * 2U));

    node_T01_state_t expected_state = std::get<3>(GetParam());

    // Act: poke the system under test
    node_T01_process_msg(&node, &cmd_rcv_msg, ((NODE_T01_LIGHT_DURATION_MS * 2U) + 1U));
    node_T01_process_movement(&node, ((NODE_T01_LIGHT_DURATION_MS * 2U) + 2U));

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, ((NODE_T01_LIGHT_DURATION_MS * 2U) + 3U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
}

INSTANTIATE_TEST_SUITE_P(NodeT01TestFixture, NodeT01ParameterizedFrontPirCommandBefore,
    testing::Values
    (
        // Silence mode
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = true,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = true,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = true }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = true,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = true,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = true }),

        // Guard mode
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = true })
    )
);



class NodeT01ParameterizedFrontPirCommandAfter : public NodeT01TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_T01_luminosity_t,
        node_msg_t,
        node_msg_t,
        node_T01_state_t
    >>
{};

TEST_P(NodeT01ParameterizedFrontPirCommandAfter, ProcessFrontPirCommandAfter)
{
    // Arrange: create and set up a system under test
    node_T01_luminosity_t luminosity    = std::get<0>(GetParam());
    node_msg_t mode_rcv_msg             = std::get<1>(GetParam());
    node_msg_t cmd_rcv_msg              = std::get<2>(GetParam());

    uint32_t next_time_ms;
    node_T01_process_luminosity(&node, &luminosity, &next_time_ms);
    node_T01_process_msg(&node, &mode_rcv_msg, (NODE_T01_LIGHT_DURATION_MS * 2U));

    node_T01_state_t expected_state = std::get<3>(GetParam());

    // Act: poke the system under test
    node_T01_process_movement(&node, ((NODE_T01_LIGHT_DURATION_MS * 2U) + 1U));
    node_T01_process_msg(&node, &cmd_rcv_msg, ((NODE_T01_LIGHT_DURATION_MS * 2U) + 2U));

    node_T01_state_t result_state;
    node_T01_get_state(&node, &result_state, ((NODE_T01_LIGHT_DURATION_MS * 2U) + 3U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.status_led_color,    expected_state.status_led_color);
    EXPECT_EQ(result_state.is_light_on,         expected_state.is_light_on);
    EXPECT_EQ(result_state.is_display_on,       expected_state.is_display_on);
    EXPECT_EQ(result_state.is_warning_led_on,   expected_state.is_warning_led_on);
    EXPECT_EQ(result_state.is_msg_to_send,      expected_state.is_msg_to_send);
}

INSTANTIATE_TEST_SUITE_P(NodeT01TestFixture, NodeT01ParameterizedFrontPirCommandAfter,
    testing::Values
    (
        // Silence mode
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = true,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = true }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = true }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = true,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = true }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = false,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = GREEN_COLOR, .is_light_on = true,
                            .is_display_on = true, .is_warning_led_on = false, .is_msg_to_send = true }),

        // Guard mode
        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = false,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = true }),

        std::make_tuple(node_T01_luminosity_t { .lux = (NODE_T01_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_T01 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_T01_state_t { .status_led_color = RED_COLOR, .is_light_on = true,
                            .is_display_on = false, .is_warning_led_on = false, .is_msg_to_send = true })
    )
);
