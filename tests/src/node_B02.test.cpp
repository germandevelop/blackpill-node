/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#include <gmock/gmock.h>

#include "node_B02.h"
#include "std_error/std_error.h"


class NodeB02TestFixture : public testing::Test
{
    protected:

        node_B02_t node;

        virtual void SetUp() override
        {
            node_B02_init(&node);
        }
};


TEST_F(NodeB02TestFixture, InitZero)
{
    // Arrange: create and set up a system under test
    node_B02_state_t expected_state;
    expected_state.is_msg_to_send               = false;
    expected_state.status_led_color             = GREEN_COLOR;
    expected_state.is_display_on                = true;
    expected_state.is_front_pir_on              = false;
    expected_state.light_strip.is_white_on      = false;
    expected_state.light_strip.is_blue_green_on = false;
    expected_state.light_strip.is_red_on        = false;
    expected_state.is_veranda_light_on          = false;
    expected_state.is_front_light_on            = false;
    expected_state.is_buzzer_on                 = false;

    // Act: poke the system under test
    node_B02_state_t result_state;
    node_B02_get_state(&node, &result_state, 0U);

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,                  expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,                expected_state.status_led_color);
    EXPECT_EQ(result_state.is_display_on,                   expected_state.is_display_on);
    EXPECT_EQ(result_state.is_front_pir_on,                 expected_state.is_front_pir_on);
    EXPECT_EQ(result_state.light_strip.is_white_on,         expected_state.light_strip.is_white_on);
    EXPECT_EQ(result_state.light_strip.is_blue_green_on,    expected_state.light_strip.is_blue_green_on);
    EXPECT_EQ(result_state.light_strip.is_red_on,           expected_state.light_strip.is_red_on);
    EXPECT_EQ(result_state.is_veranda_light_on,             expected_state.is_veranda_light_on);
    EXPECT_EQ(result_state.is_front_light_on,               expected_state.is_front_light_on);
    EXPECT_EQ(result_state.is_buzzer_on,                    expected_state.is_buzzer_on);
}

TEST_F(NodeB02TestFixture, InitStable)
{
    // Arrange: create and set up a system under test
    node_B02_state_t expected_state;
    expected_state.is_msg_to_send               = false;
    expected_state.status_led_color             = GREEN_COLOR;
    expected_state.is_display_on                = false;
    expected_state.is_front_pir_on              = false;
    expected_state.light_strip.is_white_on      = false;
    expected_state.light_strip.is_blue_green_on = false;
    expected_state.light_strip.is_red_on        = false;
    expected_state.is_veranda_light_on          = false;
    expected_state.is_front_light_on            = false;
    expected_state.is_buzzer_on                 = false;

    // Act: poke the system under test
    node_B02_state_t result_state;
    node_B02_get_state(&node, &result_state, (NODE_B02_DISPLAY_DURATION_MS + 1U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,                  expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,                expected_state.status_led_color);
    EXPECT_EQ(result_state.is_display_on,                   expected_state.is_display_on);
    EXPECT_EQ(result_state.is_front_pir_on,                 expected_state.is_front_pir_on);
    EXPECT_EQ(result_state.light_strip.is_white_on,         expected_state.light_strip.is_white_on);
    EXPECT_EQ(result_state.light_strip.is_blue_green_on,    expected_state.light_strip.is_blue_green_on);
    EXPECT_EQ(result_state.light_strip.is_red_on,           expected_state.light_strip.is_red_on);
    EXPECT_EQ(result_state.is_veranda_light_on,             expected_state.is_veranda_light_on);
    EXPECT_EQ(result_state.is_front_light_on,               expected_state.is_front_light_on);
    EXPECT_EQ(result_state.is_buzzer_on,                    expected_state.is_buzzer_on);
}


class NodeB02ParameterizedLuminosity : public NodeB02TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_B02_luminosity_t,
        bool
    >>
{};

TEST_P(NodeB02ParameterizedLuminosity, ProcessLuminosity)
{
    // Arrange: create and set up a system under test
    node_B02_luminosity_t luminosity = std::get<0>(GetParam());

    bool expected_is_dark = std::get<1>(GetParam());

    // Act: poke the system under test
    uint32_t next_time_ms;
    node_B02_process_luminosity(&node, &luminosity, &next_time_ms);

    // Assert: make unit test pass or fail
    EXPECT_EQ(node.is_dark, expected_is_dark);
}

INSTANTIATE_TEST_SUITE_P(NodeB02TestFixture, NodeB02ParameterizedLuminosity,
    testing::Values
    (
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX),       .is_valid = false },    false),
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F),.is_valid = false },    false),
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX),       .is_valid = true },     false),
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F),.is_valid = true },     true)
    )
);



class NodeB02ParameterizedTemperature : public NodeB02TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_B02_temperature_t,
        node_B02_state_t
    >>
{};

TEST_P(NodeB02ParameterizedTemperature, ProcessTemperature)
{
    // Arrange: create and set up a system under test
    node_B02_temperature_t temperature = std::get<0>(GetParam());

    node_B02_temperature_t expected_temperature = temperature;
    node_B02_state_t expected_state             = std::get<1>(GetParam());

    // Act: poke the system under test
    uint32_t next_time_ms;
    node_B02_process_temperature(&node, &temperature, &next_time_ms);

    node_B02_state_t result_state;
    node_B02_get_state(&node, &result_state, (NODE_B02_DISPLAY_DURATION_MS * 2U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(node.temperature.is_valid,    expected_temperature.is_valid);
    EXPECT_EQ(result_state.is_msg_to_send,  expected_state.is_msg_to_send);
}

INSTANTIATE_TEST_SUITE_P(NodeB02TestFixture, NodeB02ParameterizedTemperature,
    testing::Values
    (
        std::make_tuple(node_B02_temperature_t { .is_valid = false },  node_B02_state_t { .is_msg_to_send = false }),
        std::make_tuple(node_B02_temperature_t { .is_valid = true },   node_B02_state_t { .is_msg_to_send = true })
    )
);



class NodeB02ParameterizedMsgMode : public NodeB02TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_B02_luminosity_t,
        node_msg_t,
        node_B02_state_t
    >>
{};

TEST_P(NodeB02ParameterizedMsgMode, ProcessMsgMode)
{
    // Arrange: create and set up a system under test
    node_B02_luminosity_t luminosity    = std::get<0>(GetParam());
    node_msg_t rcv_msg                  = std::get<1>(GetParam());

    uint32_t next_time_ms;
    node_B02_process_luminosity(&node, &luminosity, &next_time_ms);

    node_B02_state_t expected_state = std::get<2>(GetParam());

    // Act: poke the system under test
    node_B02_process_msg(&node, &rcv_msg, (NODE_B02_LIGHT_DURATION_MS * 2U));

    node_B02_state_t result_state;
    node_B02_get_state(&node, &result_state, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 1U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,                  expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,                expected_state.status_led_color);
    EXPECT_EQ(result_state.is_display_on,                   expected_state.is_display_on);
    EXPECT_EQ(result_state.is_front_pir_on,                 expected_state.is_front_pir_on);
    EXPECT_EQ(result_state.light_strip.is_white_on,         expected_state.light_strip.is_white_on);
    EXPECT_EQ(result_state.light_strip.is_blue_green_on,    expected_state.light_strip.is_blue_green_on);
    EXPECT_EQ(result_state.light_strip.is_red_on,           expected_state.light_strip.is_red_on);
    EXPECT_EQ(result_state.is_veranda_light_on,             expected_state.is_veranda_light_on);
    EXPECT_EQ(result_state.is_front_light_on,               expected_state.is_front_light_on);
    EXPECT_EQ(result_state.is_buzzer_on,                    expected_state.is_buzzer_on);
}

INSTANTIATE_TEST_SUITE_P(NodeB02TestFixture, NodeB02ParameterizedMsgMode,
    testing::Values
    (
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false })
    )
);



class NodeB02ParameterizedMsgCommand : public NodeB02TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_B02_luminosity_t,
        node_msg_t,
        node_msg_t,
        node_B02_state_t
    >>
{};

TEST_P(NodeB02ParameterizedMsgCommand, ProcessMsgCommand)
{
    // Arrange: create and set up a system under test
    node_B02_luminosity_t luminosity    = std::get<0>(GetParam());
    node_msg_t mode_rcv_msg             = std::get<1>(GetParam());
    node_msg_t cmd_rcv_msg              = std::get<2>(GetParam());

    uint32_t next_time_ms;
    node_B02_process_luminosity(&node, &luminosity, &next_time_ms);
    node_B02_process_msg(&node, &mode_rcv_msg, (NODE_B02_LIGHT_DURATION_MS * 2U));

    node_B02_state_t expected_state = std::get<3>(GetParam());

    // Act: poke the system under test
    node_B02_process_msg(&node, &cmd_rcv_msg, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 1U));

    node_B02_state_t result_state;
    node_B02_get_state(&node, &result_state, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 2U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,                  expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,                expected_state.status_led_color);
    EXPECT_EQ(result_state.is_display_on,                   expected_state.is_display_on);
    EXPECT_EQ(result_state.is_front_pir_on,                 expected_state.is_front_pir_on);
    EXPECT_EQ(result_state.light_strip.is_white_on,         expected_state.light_strip.is_white_on);
    EXPECT_EQ(result_state.light_strip.is_blue_green_on,    expected_state.light_strip.is_blue_green_on);
    EXPECT_EQ(result_state.light_strip.is_red_on,           expected_state.light_strip.is_red_on);
    EXPECT_EQ(result_state.is_veranda_light_on,             expected_state.is_veranda_light_on);
    EXPECT_EQ(result_state.is_front_light_on,               expected_state.is_front_light_on);
    EXPECT_EQ(result_state.is_buzzer_on,                    expected_state.is_buzzer_on);
}

INSTANTIATE_TEST_SUITE_P(NodeB02TestFixture, NodeB02ParameterizedMsgCommand,
    testing::Values
    (
        // Alarm mode (0)
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = false }),

        // Guard mode (8)
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = false }),

        // Silence mode (16)
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = true, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = true, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = false })
    )
);



class NodeB02ParameterizedMsgCommandDouble : public NodeB02TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_B02_luminosity_t,
        node_msg_t,
        node_msg_t,
        node_msg_t,
        node_B02_state_t
    >>
{};

TEST_P(NodeB02ParameterizedMsgCommandDouble, ProcessMsgCommandDouble)
{
    // Arrange: create and set up a system under test
    node_B02_luminosity_t luminosity    = std::get<0>(GetParam());
    node_msg_t mode_rcv_msg             = std::get<1>(GetParam());
    node_msg_t cmd_rcv_msg_1            = std::get<2>(GetParam());
    node_msg_t cmd_rcv_msg_2            = std::get<3>(GetParam());

    uint32_t next_time_ms;
    node_B02_process_luminosity(&node, &luminosity, &next_time_ms);
    node_B02_process_msg(&node, &mode_rcv_msg, (NODE_B02_LIGHT_DURATION_MS * 2U));

    node_B02_state_t expected_state = std::get<4>(GetParam());

    // Act: poke the system under test
    node_B02_process_msg(&node, &cmd_rcv_msg_1, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 1U));
    node_B02_process_msg(&node, &cmd_rcv_msg_2, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 2U));

    node_B02_state_t result_state;
    node_B02_get_state(&node, &result_state, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 3U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,                  expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,                expected_state.status_led_color);
    EXPECT_EQ(result_state.is_display_on,                   expected_state.is_display_on);
    EXPECT_EQ(result_state.is_front_pir_on,                 expected_state.is_front_pir_on);
    EXPECT_EQ(result_state.light_strip.is_white_on,         expected_state.light_strip.is_white_on);
    EXPECT_EQ(result_state.light_strip.is_blue_green_on,    expected_state.light_strip.is_blue_green_on);
    EXPECT_EQ(result_state.light_strip.is_red_on,           expected_state.light_strip.is_red_on);
    EXPECT_EQ(result_state.is_veranda_light_on,             expected_state.is_veranda_light_on);
    EXPECT_EQ(result_state.is_front_light_on,               expected_state.is_front_light_on);
    EXPECT_EQ(result_state.is_buzzer_on,                    expected_state.is_buzzer_on);
}

INSTANTIATE_TEST_SUITE_P(NodeB02TestFixture, NodeB02ParameterizedMsgCommandDouble,
    testing::Values
    (
        // Silence mode
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = true, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = true, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = true, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = true, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = false }),

        // Guard mode
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = false })
    )
);



TEST_F(NodeB02TestFixture, NodeB02DoorPir)
{
    // Arrange: create and set up a system under test
    node_B02_luminosity_t luminosity;
    luminosity.lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F);
    luminosity.is_valid = true;

    uint32_t next_time_ms;
    node_B02_process_luminosity(&node, &luminosity, &next_time_ms);

    node_B02_state_t expected_state;
    expected_state.is_msg_to_send               = true;
    expected_state.status_led_color             = GREEN_COLOR;
    expected_state.is_display_on                = false;
    expected_state.is_front_pir_on              = true;
    expected_state.light_strip.is_white_on      = false;
    expected_state.light_strip.is_blue_green_on = false;
    expected_state.light_strip.is_red_on        = false;
    expected_state.is_veranda_light_on          = false;
    expected_state.is_front_light_on            = false;
    expected_state.is_buzzer_on                 = false;

    // Act: poke the system under test
    node_B02_process_door_movement(&node, (NODE_B02_LIGHT_DURATION_MS * 2U));
    node_B02_process_door_movement(&node, (NODE_B02_LIGHT_DURATION_MS * 3U));

    node_B02_state_t result_state;
    node_B02_get_state(&node, &result_state, ((NODE_B02_LIGHT_DURATION_MS * 3U) + 1U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,                  expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,                expected_state.status_led_color);
    EXPECT_EQ(result_state.is_display_on,                   expected_state.is_display_on);
    EXPECT_EQ(result_state.is_front_pir_on,                 expected_state.is_front_pir_on);
    EXPECT_EQ(result_state.light_strip.is_white_on,         expected_state.light_strip.is_white_on);
    EXPECT_EQ(result_state.light_strip.is_blue_green_on,    expected_state.light_strip.is_blue_green_on);
    EXPECT_EQ(result_state.light_strip.is_red_on,           expected_state.light_strip.is_red_on);
    EXPECT_EQ(result_state.is_veranda_light_on,             expected_state.is_veranda_light_on);
    EXPECT_EQ(result_state.is_front_light_on,               expected_state.is_front_light_on);
    EXPECT_EQ(result_state.is_buzzer_on,                    expected_state.is_buzzer_on);
}



class NodeB02ParameterizedDoorPirMode : public NodeB02TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_B02_luminosity_t,
        node_msg_t,
        node_B02_state_t
    >>
{};

TEST_P(NodeB02ParameterizedDoorPirMode, ProcessDoorPirMode)
{
    // Arrange: create and set up a system under test
    node_B02_luminosity_t luminosity    = std::get<0>(GetParam());
    node_msg_t rcv_msg                  = std::get<1>(GetParam());

    uint32_t next_time_ms;
    node_B02_process_luminosity(&node, &luminosity, &next_time_ms);
    node_B02_process_msg(&node, &rcv_msg, (NODE_B02_LIGHT_DURATION_MS * 2U));

    node_B02_state_t expected_state = std::get<2>(GetParam());

    // Act: poke the system under test
    node_B02_process_door_movement(&node, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 1U));

    node_B02_state_t result_state;
    node_B02_get_state(&node, &result_state, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 2U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,                  expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,                expected_state.status_led_color);
    EXPECT_EQ(result_state.is_display_on,                   expected_state.is_display_on);
    EXPECT_EQ(result_state.is_front_pir_on,                 expected_state.is_front_pir_on);
    EXPECT_EQ(result_state.light_strip.is_white_on,         expected_state.light_strip.is_white_on);
    EXPECT_EQ(result_state.light_strip.is_blue_green_on,    expected_state.light_strip.is_blue_green_on);
    EXPECT_EQ(result_state.light_strip.is_red_on,           expected_state.light_strip.is_red_on);
    EXPECT_EQ(result_state.is_veranda_light_on,             expected_state.is_veranda_light_on);
    EXPECT_EQ(result_state.is_front_light_on,               expected_state.is_front_light_on);
    EXPECT_EQ(result_state.is_buzzer_on,                    expected_state.is_buzzer_on);
}

INSTANTIATE_TEST_SUITE_P(NodeB02TestFixture, NodeB02ParameterizedDoorPirMode,
    testing::Values
    (
        // Alarm mode
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = false }),

        // Guard mode
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = true }),

        // Silence mode
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = true, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = true })
    )
);


class NodeB02ParameterizedDoorPirCommandBefore : public NodeB02TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_B02_luminosity_t,
        node_msg_t,
        node_msg_t,
        node_B02_state_t
    >>
{};

TEST_P(NodeB02ParameterizedDoorPirCommandBefore, ProcessDoorPirCommandBefore)
{
    // Arrange: create and set up a system under test
    node_B02_luminosity_t luminosity    = std::get<0>(GetParam());
    node_msg_t mode_rcv_msg             = std::get<1>(GetParam());
    node_msg_t cmd_rcv_msg              = std::get<2>(GetParam());

    uint32_t next_time_ms;
    node_B02_process_luminosity(&node, &luminosity, &next_time_ms);
    node_B02_process_msg(&node, &mode_rcv_msg, (NODE_B02_LIGHT_DURATION_MS * 2U));

    node_B02_state_t expected_state = std::get<3>(GetParam());

    // Act: poke the system under test
    node_B02_process_msg(&node, &cmd_rcv_msg, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 1U));
    node_B02_process_door_movement(&node, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 2U));

    node_B02_state_t result_state;
    node_B02_get_state(&node, &result_state, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 3U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,                  expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,                expected_state.status_led_color);
    EXPECT_EQ(result_state.is_display_on,                   expected_state.is_display_on);
    EXPECT_EQ(result_state.is_front_pir_on,                 expected_state.is_front_pir_on);
    EXPECT_EQ(result_state.light_strip.is_white_on,         expected_state.light_strip.is_white_on);
    EXPECT_EQ(result_state.light_strip.is_blue_green_on,    expected_state.light_strip.is_blue_green_on);
    EXPECT_EQ(result_state.light_strip.is_red_on,           expected_state.light_strip.is_red_on);
    EXPECT_EQ(result_state.is_veranda_light_on,             expected_state.is_veranda_light_on);
    EXPECT_EQ(result_state.is_front_light_on,               expected_state.is_front_light_on);
    EXPECT_EQ(result_state.is_buzzer_on,                    expected_state.is_buzzer_on);
}

INSTANTIATE_TEST_SUITE_P(NodeB02TestFixture, NodeB02ParameterizedDoorPirCommandBefore,
    testing::Values
    (
        // Guard mode
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = true }),

        // Silence mode
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = true, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = true, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = true, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = true, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = true })
    )
);



class NodeB02ParameterizedDoorPirCommandAfter : public NodeB02TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_B02_luminosity_t,
        node_msg_t,
        node_msg_t,
        node_B02_state_t
    >>
{};

TEST_P(NodeB02ParameterizedDoorPirCommandAfter, ProcessDoorPirCommandAfter)
{
    // Arrange: create and set up a system under test
    node_B02_luminosity_t luminosity    = std::get<0>(GetParam());
    node_msg_t mode_rcv_msg             = std::get<1>(GetParam());
    node_msg_t cmd_rcv_msg              = std::get<2>(GetParam());

    uint32_t next_time_ms;
    node_B02_process_luminosity(&node, &luminosity, &next_time_ms);
    node_B02_process_msg(&node, &mode_rcv_msg, (NODE_B02_LIGHT_DURATION_MS * 2U));

    node_B02_state_t expected_state = std::get<3>(GetParam());

    // Act: poke the system under test
    node_B02_process_door_movement(&node, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 1U));
    node_B02_process_msg(&node, &cmd_rcv_msg, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 2U));

    node_B02_state_t result_state;
    node_B02_get_state(&node, &result_state, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 3U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,                  expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,                expected_state.status_led_color);
    EXPECT_EQ(result_state.is_display_on,                   expected_state.is_display_on);
    EXPECT_EQ(result_state.is_front_pir_on,                 expected_state.is_front_pir_on);
    EXPECT_EQ(result_state.light_strip.is_white_on,         expected_state.light_strip.is_white_on);
    EXPECT_EQ(result_state.light_strip.is_blue_green_on,    expected_state.light_strip.is_blue_green_on);
    EXPECT_EQ(result_state.light_strip.is_red_on,           expected_state.light_strip.is_red_on);
    EXPECT_EQ(result_state.is_veranda_light_on,             expected_state.is_veranda_light_on);
    EXPECT_EQ(result_state.is_front_light_on,               expected_state.is_front_light_on);
    EXPECT_EQ(result_state.is_buzzer_on,                    expected_state.is_buzzer_on);
}

INSTANTIATE_TEST_SUITE_P(NodeB02TestFixture, NodeB02ParameterizedDoorPirCommandAfter,
    testing::Values
    (
        // Guard mode
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = true }),

        // Silence mode
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = true, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = true, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = true, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = true })
    )
);



TEST_F(NodeB02TestFixture, NodeB02VerandaPir)
{
    // Arrange: create and set up a system under test
    node_B02_luminosity_t luminosity;
    luminosity.lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F);
    luminosity.is_valid = true;

    uint32_t next_time_ms;
    node_B02_process_luminosity(&node, &luminosity, &next_time_ms);

    node_B02_state_t expected_state;
    expected_state.is_msg_to_send               = false;
    expected_state.status_led_color             = GREEN_COLOR;
    expected_state.is_display_on                = false;
    expected_state.is_front_pir_on              = true;
    expected_state.light_strip.is_white_on      = false;
    expected_state.light_strip.is_blue_green_on = false;
    expected_state.light_strip.is_red_on        = false;
    expected_state.is_veranda_light_on          = false;
    expected_state.is_front_light_on            = false;
    expected_state.is_buzzer_on                 = false;

    // Act: poke the system under test
    node_B02_process_veranda_movement(&node, (NODE_B02_LIGHT_DURATION_MS * 2U));
    node_B02_process_veranda_movement(&node, (NODE_B02_LIGHT_DURATION_MS * 3U));

    node_B02_state_t result_state;
    node_B02_get_state(&node, &result_state, ((NODE_B02_LIGHT_DURATION_MS * 3U) + 1U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,                  expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,                expected_state.status_led_color);
    EXPECT_EQ(result_state.is_display_on,                   expected_state.is_display_on);
    EXPECT_EQ(result_state.is_front_pir_on,                 expected_state.is_front_pir_on);
    EXPECT_EQ(result_state.light_strip.is_white_on,         expected_state.light_strip.is_white_on);
    EXPECT_EQ(result_state.light_strip.is_blue_green_on,    expected_state.light_strip.is_blue_green_on);
    EXPECT_EQ(result_state.light_strip.is_red_on,           expected_state.light_strip.is_red_on);
    EXPECT_EQ(result_state.is_veranda_light_on,             expected_state.is_veranda_light_on);
    EXPECT_EQ(result_state.is_front_light_on,               expected_state.is_front_light_on);
    EXPECT_EQ(result_state.is_buzzer_on,                    expected_state.is_buzzer_on);
}


class NodeB02ParameterizedVerandaPirMode : public NodeB02TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_B02_luminosity_t,
        node_msg_t,
        node_B02_state_t
    >>
{};

TEST_P(NodeB02ParameterizedVerandaPirMode, ProcessVerandaPirMode)
{
    // Arrange: create and set up a system under test
    node_B02_luminosity_t luminosity    = std::get<0>(GetParam());
    node_msg_t rcv_msg                  = std::get<1>(GetParam());

    uint32_t next_time_ms;
    node_B02_process_luminosity(&node, &luminosity, &next_time_ms);
    node_B02_process_msg(&node, &rcv_msg, (NODE_B02_LIGHT_DURATION_MS * 2U));

    node_B02_state_t expected_state = std::get<2>(GetParam());

    // Act: poke the system under test
    node_B02_process_veranda_movement(&node, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 1U));

    node_B02_state_t result_state;
    node_B02_get_state(&node, &result_state, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 2U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,                  expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,                expected_state.status_led_color);
    EXPECT_EQ(result_state.is_display_on,                   expected_state.is_display_on);
    EXPECT_EQ(result_state.is_front_pir_on,                 expected_state.is_front_pir_on);
    EXPECT_EQ(result_state.light_strip.is_white_on,         expected_state.light_strip.is_white_on);
    EXPECT_EQ(result_state.light_strip.is_blue_green_on,    expected_state.light_strip.is_blue_green_on);
    EXPECT_EQ(result_state.light_strip.is_red_on,           expected_state.light_strip.is_red_on);
    EXPECT_EQ(result_state.is_veranda_light_on,             expected_state.is_veranda_light_on);
    EXPECT_EQ(result_state.is_front_light_on,               expected_state.is_front_light_on);
    EXPECT_EQ(result_state.is_buzzer_on,                    expected_state.is_buzzer_on);
}

INSTANTIATE_TEST_SUITE_P(NodeB02TestFixture, NodeB02ParameterizedVerandaPirMode,
    testing::Values
    (
        // Alarm mode
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(ALARM_MODE) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = false,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = false }),

        // Guard mode
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = true }),

        // Silence mode
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = true, .is_front_pir_on = false,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(SILENCE_MODE) },
                        node_B02_state_t { .status_led_color = GREEN_COLOR, .is_display_on = true, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = false })
    )
);



class NodeB02ParameterizedVerandaPirCommandBefore : public NodeB02TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_B02_luminosity_t,
        node_msg_t,
        node_msg_t,
        node_B02_state_t
    >>
{};

TEST_P(NodeB02ParameterizedVerandaPirCommandBefore, ProcessVerandaPirCommandBefore)
{
    // Arrange: create and set up a system under test
    node_B02_luminosity_t luminosity    = std::get<0>(GetParam());
    node_msg_t mode_rcv_msg             = std::get<1>(GetParam());
    node_msg_t cmd_rcv_msg              = std::get<2>(GetParam());

    uint32_t next_time_ms;
    node_B02_process_luminosity(&node, &luminosity, &next_time_ms);
    node_B02_process_msg(&node, &mode_rcv_msg, (NODE_B02_LIGHT_DURATION_MS * 2U));

    node_B02_state_t expected_state = std::get<3>(GetParam());

    // Act: poke the system under test
    node_B02_process_msg(&node, &cmd_rcv_msg, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 1U));
    node_B02_process_veranda_movement(&node, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 2U));

    node_B02_state_t result_state;
    node_B02_get_state(&node, &result_state, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 3U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,                  expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,                expected_state.status_led_color);
    EXPECT_EQ(result_state.is_display_on,                   expected_state.is_display_on);
    EXPECT_EQ(result_state.is_front_pir_on,                 expected_state.is_front_pir_on);
    EXPECT_EQ(result_state.light_strip.is_white_on,         expected_state.light_strip.is_white_on);
    EXPECT_EQ(result_state.light_strip.is_blue_green_on,    expected_state.light_strip.is_blue_green_on);
    EXPECT_EQ(result_state.light_strip.is_red_on,           expected_state.light_strip.is_red_on);
    EXPECT_EQ(result_state.is_veranda_light_on,             expected_state.is_veranda_light_on);
    EXPECT_EQ(result_state.is_front_light_on,               expected_state.is_front_light_on);
    EXPECT_EQ(result_state.is_buzzer_on,                    expected_state.is_buzzer_on);
}

INSTANTIATE_TEST_SUITE_P(NodeB02TestFixture, NodeB02ParameterizedVerandaPirCommandBefore,
    testing::Values
    (
        // Guard mode
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = false }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = true })
    )
);



class NodeB02ParameterizedVerandaPirCommandAfter : public NodeB02TestFixture, public testing::WithParamInterface
    <std::tuple<
        node_B02_luminosity_t,
        node_msg_t,
        node_msg_t,
        node_B02_state_t
    >>
{};

TEST_P(NodeB02ParameterizedVerandaPirCommandAfter, ProcessVerandaPirCommandAfter)
{
    // Arrange: create and set up a system under test
    node_B02_luminosity_t luminosity    = std::get<0>(GetParam());
    node_msg_t mode_rcv_msg             = std::get<1>(GetParam());
    node_msg_t cmd_rcv_msg              = std::get<2>(GetParam());

    uint32_t next_time_ms;
    node_B02_process_luminosity(&node, &luminosity, &next_time_ms);
    node_B02_process_msg(&node, &mode_rcv_msg, (NODE_B02_LIGHT_DURATION_MS * 2U));

    node_B02_state_t expected_state = std::get<3>(GetParam());

    // Act: poke the system under test
    node_B02_process_veranda_movement(&node, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 1U));
    node_B02_process_msg(&node, &cmd_rcv_msg, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 2U));

    node_B02_state_t result_state;
    node_B02_get_state(&node, &result_state, ((NODE_B02_LIGHT_DURATION_MS * 2U) + 3U));

    // Assert: make unit test pass or fail
    EXPECT_EQ(result_state.is_msg_to_send,                  expected_state.is_msg_to_send);
    EXPECT_EQ(result_state.status_led_color,                expected_state.status_led_color);
    EXPECT_EQ(result_state.is_display_on,                   expected_state.is_display_on);
    EXPECT_EQ(result_state.is_front_pir_on,                 expected_state.is_front_pir_on);
    EXPECT_EQ(result_state.light_strip.is_white_on,         expected_state.light_strip.is_white_on);
    EXPECT_EQ(result_state.light_strip.is_blue_green_on,    expected_state.light_strip.is_blue_green_on);
    EXPECT_EQ(result_state.light_strip.is_red_on,           expected_state.light_strip.is_red_on);
    EXPECT_EQ(result_state.is_veranda_light_on,             expected_state.is_veranda_light_on);
    EXPECT_EQ(result_state.is_front_light_on,               expected_state.is_front_light_on);
    EXPECT_EQ(result_state.is_buzzer_on,                    expected_state.is_buzzer_on);
}

INSTANTIATE_TEST_SUITE_P(NodeB02TestFixture, NodeB02ParameterizedVerandaPirCommandAfter,
    testing::Values
    (
        // Guard mode
        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_LIGHT, .value_0 = (int32_t)(LIGHT_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_ON) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = true },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = true, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = false, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = false, .is_front_light_on = false, .is_buzzer_on = false, .is_msg_to_send = true }),

        std::make_tuple(node_B02_luminosity_t { .lux = (NODE_B02_DARKNESS_LEVEL_LUX - 1.0F), .is_valid = true },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_MODE, .value_0 = (int32_t)(GUARD_MODE) },
                        node_msg_t { .header { .dest_array { [0] = NODE_B02 }, .dest_array_size = 1U },
                            .cmd_id = SET_INTRUSION, .value_0 = (int32_t)(INTRUSION_OFF) },
                        node_B02_state_t { .status_led_color = RED_COLOR, .is_display_on = false, .is_front_pir_on = true,
                            .light_strip { .is_white_on = true, .is_blue_green_on = false, .is_red_on = false },
                            .is_veranda_light_on = true, .is_front_light_on = true, .is_buzzer_on = false, .is_msg_to_send = true })
    )
);
