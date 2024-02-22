/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#include "node_T01.h"

#include <assert.h>
#include <string.h>

#include "std_error/std_error.h"


#define DEFAULT_ERROR_TEXT  "Node T01 error"
#define MESSAGE_ERROR_TEXT  "Node T01 message error"

#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


static void node_T01_update_state (node_T01_t * const self, uint32_t time_ms);

void node_T01_init (node_T01_t * const self)
{
    assert(self != NULL);

    self->state.is_msg_to_send      = false;
    self->state.status_led_color    = GREEN_COLOR;
    self->state.is_light_on         = false;
    self->state.is_display_on       = false;
    self->state.is_warning_led_on   = false;

    self->mode          = SILENCE;
    self->is_dark       = false;
    self->is_door_open  = false;

    self->light_and_display_start_time_ms = 0U;

    self->humidity.is_valid = false;

    self->send_msg_buffer_size = 0U;

    return;
}

void node_T01_get_state (node_T01_t * const self,
                        node_T01_state_t * const state,
                        uint32_t time_ms)
{
    assert(self     != NULL);
    assert(state    != NULL);

    node_T01_update_state(self, time_ms);

    *state = self->state;

    return;
}

void node_T01_update_state (node_T01_t * const self,
                            uint32_t time_ms)
{
    assert(self != NULL);

    // Update light and display state
    const uint32_t duration_ms = time_ms - self->light_and_display_start_time_ms;

    if (self->mode == ALARM)
    {
        if (self->is_dark == true)
        {
            self->state.is_light_on = true;
        }
        else
        {
            self->state.is_light_on = false;
        }
        self->state.is_display_on       = false;
        self->state.is_warning_led_on   = true;
    }

    else if (self->mode == GUARD)
    {
        if (duration_ms > NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS)
        {
            self->state.is_light_on = false;
        }
        else
        {
            if (self->is_dark == true)
            {
                self->state.is_light_on = true;
            }
            else
            {
                self->state.is_light_on = false;
            }
        }
        self->state.is_display_on       = false;
        self->state.is_warning_led_on   = false;
    }

    else if (self->mode == SILENCE)
    {
        if (duration_ms > NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS)
        {
            self->state.is_light_on     = false;
            self->state.is_display_on   = false;
        }
        else
        {
            if (self->is_dark == true)
            {
                self->state.is_light_on = true;
            }
            else
            {
                self->state.is_light_on = false;
            }
            self->state.is_display_on = true;
        }

        self->state.is_warning_led_on = false;

        if (self->humidity.is_valid == true)
        {
            if (self->is_door_open == true)
            {
                if (self->humidity.temperature_C < NODE_T01_LOW_TEMPERATURE_C)  // Without hysteresis for now
                {
                    self->state.is_warning_led_on = true;
                }
            }
            else
            {
                if (self->humidity.temperature_C > NODE_T01_HIGH_TEMPERATURE_C) // Without hysteresis for now
                {
                    self->state.is_warning_led_on = true;
                }
            }
        }
    }

    // Update status LED color
    if ((self->mode == GUARD) || (self->mode == ALARM))
    {
        self->state.status_led_color = RED_COLOR;
    }
    else
    {
        self->state.status_led_color = GREEN_COLOR;
    }

    // Update message state
    if (self->send_msg_buffer_size != 0U)
    {
        self->state.is_msg_to_send = true;
    }
    else
    {
        self->state.is_msg_to_send = false;
    }

    return;
}

void node_T01_process_luminosity (  node_T01_t * const self,
                                    node_T01_luminosity_t const * const data,
                                    uint32_t * const next_time_ms)
{
    assert(self         != NULL);
    assert(data         != NULL);
    assert(next_time_ms != NULL);

    *next_time_ms = NODE_T01_LUMINOSITY_PERIOD_MS;

    if (data->is_valid == true)
    {
        if (data->adc > NODE_T01_DARKNESS_LEVEL_ADC)
        {
            self->is_dark = true;
        }
        else
        {
            self->is_dark = false;
        }
    }
    else
    {
        self->is_dark = false;
    }

    return;
}

void node_T01_process_humidity (node_T01_t * const self,
                                node_T01_humidity_t const * const data,
                                uint32_t * const next_time_ms)
{
    assert(self         != NULL);
    assert(data         != NULL);
    assert(next_time_ms != NULL);

    *next_time_ms = NODE_T01_HUMIDITY_PERIOD_MS;

    self->humidity = *data;

    if (self->humidity.is_valid == true)
    {
        if (self->send_msg_buffer_size != ARRAY_SIZE(self->send_msg_buffer))
        {
            const size_t i = self->send_msg_buffer_size;
            size_t j = 0U;

            self->send_msg_buffer[i].header.source = NODE_T01;
            self->send_msg_buffer[i].header.dest_array[j] = NODE_B01;
            ++j;
            self->send_msg_buffer[i].header.dest_array_size = j;

            self->send_msg_buffer[i].cmd_id = UPDATE_TEMPERATURE;
            self->send_msg_buffer[i].value_0 = (uint32_t)(self->humidity.pressure_hPa);
            self->send_msg_buffer[i].value_1 = (int32_t)(self->humidity.humidity_pct);
            self->send_msg_buffer[i].value_2 = self->humidity.temperature_C;

            ++self->send_msg_buffer_size;
        }
    }

    return;
}

void node_T01_process_reed_switch ( node_T01_t * const self,
                                    bool is_reed_switch_open,
                                    uint32_t * const next_time_ms)
{
    assert(self         != NULL);
    assert(next_time_ms != NULL);

    *next_time_ms = NODE_T01_DOOR_STATE_PERIOD_MS;

    if (is_reed_switch_open == true)
    {
        self->is_door_open = true;
    }
    else
    {
        self->is_door_open = false;
    }

    if (self->send_msg_buffer_size != ARRAY_SIZE(self->send_msg_buffer))
    {
        const size_t i = self->send_msg_buffer_size;
        size_t j = 0U;

        self->send_msg_buffer[i].header.source = NODE_T01;
        self->send_msg_buffer[i].header.dest_array[j] = NODE_B01;
        ++j;
        self->send_msg_buffer[i].header.dest_array_size = j;

        self->send_msg_buffer[i].cmd_id = UPDATE_DOOR_STATE;

        if (self->is_door_open == true)
        {
            self->send_msg_buffer[i].value_0 = 1U;
        }
        else
        {
            self->send_msg_buffer[i].value_0 = 0U;
        }

        ++self->send_msg_buffer_size;
    }

    return;
}

void node_T01_process_remote_button (node_T01_t * const self,
                                    board_remote_button_t remote_button)
{
    UNUSED(remote_button);

    assert(self != NULL);

    // Do nothing

    return;
}

void node_T01_process_front_pir (node_T01_t * const self,
                                uint32_t time_ms)
{
    assert(self != NULL);

    const uint32_t duration_ms = time_ms - self->light_and_display_start_time_ms;

    if (duration_ms > NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS)
    {
        self->light_and_display_start_time_ms = time_ms;

        if (self->send_msg_buffer_size != ARRAY_SIZE(self->send_msg_buffer))
        {
            if ((self->mode == SILENCE) && (self->is_dark == true))
            {
                const size_t i = self->send_msg_buffer_size;
                size_t j = 0U;

                self->send_msg_buffer[i].header.source = NODE_T01;
                self->send_msg_buffer[i].header.dest_array[j] = NODE_B02;
                ++j;
                self->send_msg_buffer[i].header.dest_array_size = j;

                self->send_msg_buffer[i].cmd_id = SET_LIGHT;
                self->send_msg_buffer[i].value_0 = (uint32_t)(LIGHT_ON);

                ++self->send_msg_buffer_size;
            }
            else if (self->mode == GUARD)
            {
                const size_t i = self->send_msg_buffer_size;
                size_t j = 0U;

                self->send_msg_buffer[i].header.source = NODE_T01;
                self->send_msg_buffer[i].header.dest_array[j] = NODE_B01;
                ++j;
                self->send_msg_buffer[i].header.dest_array[j] = NODE_B02;
                ++j;
                self->send_msg_buffer[i].header.dest_array_size = j;

                self->send_msg_buffer[i].cmd_id = SET_INTRUSION;
                self->send_msg_buffer[i].value_0 = (uint32_t)(INTRUSION_ON);

                ++self->send_msg_buffer_size;
            }
        }
    }

    return;
}

void node_T01_get_light_data (  node_T01_t const * const self,
                                uint32_t * const disable_time_ms)
{
    assert(self             != NULL);
    assert(disable_time_ms  != NULL);

    *disable_time_ms = NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS;

    return;
}

void node_T01_get_display_data (node_T01_t const * const self,
                                node_T01_humidity_t * const data,
                                uint32_t * const disable_time_ms)
{
    assert(self             != NULL);
    assert(data             != NULL);
    assert(disable_time_ms  != NULL);

    *disable_time_ms = NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS;

    *data = self->humidity;

    return;
}

void node_T01_process_rcv_msg ( node_T01_t * const self,
                                node_msg_t const * const rcv_msg,
                                uint32_t time_ms)
{
    assert(self     != NULL);
    assert(rcv_msg  != NULL);

    if (rcv_msg->cmd_id == SET_MODE)
    {
        self->mode = (node_mode_id_t)(rcv_msg->value_0);

        self->light_and_display_start_time_ms = 0U;
    }
    else if (rcv_msg->cmd_id == SET_INTRUSION) 
    {
        const node_intrusion_id_t intrusion_id = (node_intrusion_id_t)(rcv_msg->value_0);

        if (intrusion_id == INTRUSION_ON)
        {
            const uint32_t duration_ms = time_ms - self->light_and_display_start_time_ms;

            if (duration_ms > NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS)
            {
                self->light_and_display_start_time_ms = time_ms;
            }
        }
        else if (intrusion_id == INTRUSION_OFF)
        {
            self->light_and_display_start_time_ms = 0U;
        }
    }
    else if (rcv_msg->cmd_id == SET_LIGHT) 
    {
        const node_light_id_t light_id = (node_light_id_t)(rcv_msg->value_0);

        if (light_id == LIGHT_ON)
        {
            const uint32_t duration_ms = time_ms - self->light_and_display_start_time_ms;

            if (duration_ms > NODE_T01_LIGHT_AND_DISPLAY_DURATION_MS)
            {
                self->light_and_display_start_time_ms = time_ms;
            }
        }
        else if (light_id == LIGHT_OFF)
        {
            self->light_and_display_start_time_ms = 0U;
        }
    }

    return;
}

int node_T01_get_msg (  node_T01_t * const self,
                        node_msg_t *msg,
                        std_error_t * const error)
{
    if (self->send_msg_buffer_size == 0U)
    {
        std_error_catch_custom(error, STD_FAILURE, MESSAGE_ERROR_TEXT, __FILE__, __LINE__);

        return STD_FAILURE;
    }

    --self->send_msg_buffer_size;
    memcpy((void*)(msg), (const void*)(&self->send_msg_buffer[self->send_msg_buffer_size]), sizeof(node_msg_t));

    return STD_SUCCESS;
}