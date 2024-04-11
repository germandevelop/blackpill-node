/************************************************************
 *   Author : German Mundinger
 *   Date   : 2024
 ************************************************************/

#include "node_B02.h"

#include <assert.h>
#include <string.h>

#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


static void node_B02_update_time (node_B02_t * const self, uint32_t time_ms);
static void node_B02_update_state (node_B02_t * const self, uint32_t time_ms);

void node_B02_init (node_B02_t * const self)
{
    assert(self != NULL);

    self->id = NODE_B02;

    self->state.is_msg_to_send                  = false;
    self->state.status_led_color                = GREEN_COLOR;
    self->state.is_display_on                   = false;
    self->state.is_front_pir_on                 = false;
    self->state.light_strip.is_white_on         = false;
    self->state.light_strip.is_blue_green_on    = false;
    self->state.light_strip.is_red_on           = false;
    self->state.is_veranda_light_on             = false;
    self->state.is_front_light_on               = false;
    self->state.is_buzzer_on                    = false;

    self->mode      = SILENCE_MODE;
    self->is_dark   = false;

    self->light_start_time_ms       = 0U;
    self->display_start_time_ms     = 0U;
    self->intrusion_start_time_ms   = 0U;

    self->temperature.is_valid = false;

    self->send_msg_buffer_size = 0U;

    return;
}

void node_B02_get_state (node_B02_t * const self,
                        node_B02_state_t * const state,
                        uint32_t time_ms)
{
    assert(self     != NULL);
    assert(state    != NULL);

    node_B02_update_state(self, time_ms);

    *state = self->state;

    return;
}

void node_B02_update_time (node_B02_t * const self, uint32_t time_ms)
{
    if (self->light_start_time_ms > time_ms)
    {
        self->light_start_time_ms = 0U;
    }

    if (self->display_start_time_ms > time_ms)
    {
        self->display_start_time_ms = 0U;
    }

    if (self->intrusion_start_time_ms > time_ms)
    {
        self->intrusion_start_time_ms = 0U;
    }

    return;
}

void node_B02_update_state (node_B02_t * const self,
                            uint32_t time_ms)
{
    assert(self != NULL);

    // Update light and display state
    node_B02_update_time(self, time_ms);
    const uint32_t light_duration_ms        = time_ms - self->light_start_time_ms;
    const uint32_t display_duration_ms      = time_ms - self->display_start_time_ms;
    const uint32_t intrusion_duration_ms    = time_ms - self->intrusion_start_time_ms;

    if (self->mode == ALARM_MODE)
    {
        if (self->is_dark == true)
        {
            self->state.light_strip.is_white_on = true;
            self->state.is_veranda_light_on     = true;
            self->state.is_front_light_on       = true;
        }
        else
        {
            self->state.light_strip.is_white_on = false;
            self->state.is_veranda_light_on     = false;
            self->state.is_front_light_on       = false;
        }
        self->state.is_display_on                   = false;
        self->state.is_front_pir_on                 = false;
        self->state.light_strip.is_blue_green_on    = false;
        self->state.light_strip.is_red_on           = true;
        self->state.is_buzzer_on                    = true;
    }

    else if (self->mode == GUARD_MODE)
    {
        if (light_duration_ms > NODE_B02_LIGHT_DURATION_MS)
        {
            self->state.light_strip.is_white_on = false;
            self->state.is_veranda_light_on     = false;
            self->state.is_front_light_on       = false;
        }
        else
        {
            if (self->is_dark == true)
            {
                self->state.light_strip.is_white_on = true;
                self->state.is_veranda_light_on     = true;
                self->state.is_front_light_on       = true;
            }
            else
            {
                self->state.light_strip.is_white_on = false;
                self->state.is_veranda_light_on     = false;
                self->state.is_front_light_on       = false;
            }
        }

        if (intrusion_duration_ms > NODE_B02_INTRUSION_DURATION_MS)
        {
            self->state.light_strip.is_red_on   = false;
            self->state.is_buzzer_on            = false;
        }
        else
        {
            self->state.light_strip.is_red_on   = true;
            self->state.is_buzzer_on            = true;
        }

        self->state.is_display_on                   = false;
        self->state.is_front_pir_on                 = true;
        self->state.light_strip.is_blue_green_on    = false;
    }

    else if (self->mode == SILENCE_MODE)
    {
        if (light_duration_ms > NODE_B02_LIGHT_DURATION_MS)
        {
            self->state.light_strip.is_white_on         = false;
            self->state.light_strip.is_blue_green_on    = false;
            self->state.is_veranda_light_on             = false;
            self->state.is_front_light_on               = false;
        }
        else
        {
            if (self->is_dark == true)
            {
                self->state.light_strip.is_white_on         = true;
                self->state.light_strip.is_blue_green_on    = true;
                self->state.is_veranda_light_on             = true;
                self->state.is_front_light_on               = true;
            }
            else
            {
                self->state.light_strip.is_white_on         = false;
                self->state.light_strip.is_blue_green_on    = false;
                self->state.is_veranda_light_on             = false;
                self->state.is_front_light_on               = false;
            }
        }

        if (display_duration_ms > NODE_B02_DISPLAY_DURATION_MS)
        {
            self->state.is_display_on = false;
        }
        else
        {
            self->state.is_display_on = true;
        }

        if (self->is_dark == true)
        {
            self->state.is_front_pir_on = true;
        }
        else
        {
            self->state.is_front_pir_on = false;
        }

        self->state.light_strip.is_red_on   = false;
        self->state.is_buzzer_on            = false;
    }

    // Update status LED color
    if ((self->mode == GUARD_MODE) || (self->mode == ALARM_MODE))
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

void node_B02_process_luminosity (  node_B02_t * const self,
                                    node_B02_luminosity_t const * const data,
                                    uint32_t * const next_time_ms)
{
    assert(self         != NULL);
    assert(data         != NULL);
    assert(next_time_ms != NULL);

    *next_time_ms = NODE_B02_LUMINOSITY_PERIOD_MS;

    if (data->is_valid == true)
    {
        if (data->lux < NODE_B02_DARKNESS_LEVEL_LUX)
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

void node_B02_process_temperature ( node_B02_t * const self,
                                    node_B02_temperature_t const * const data,
                                    uint32_t * const next_time_ms)
{
    assert(self         != NULL);
    assert(data         != NULL);
    assert(next_time_ms != NULL);

    *next_time_ms = NODE_B02_TEMPERATURE_PERIOD_MS;

    self->temperature = *data;

    if (self->temperature.is_valid == true)
    {
        if (self->send_msg_buffer_size != ARRAY_SIZE(self->send_msg_buffer))
        {
            const size_t i = self->send_msg_buffer_size;
            size_t j = 0U;

            self->send_msg_buffer[i].header.source = self->id;
            self->send_msg_buffer[i].header.dest_array[j] = NODE_B01;
            ++j;
            self->send_msg_buffer[i].header.dest_array_size = j;

            self->send_msg_buffer[i].cmd_id = UPDATE_TEMPERATURE;
            self->send_msg_buffer[i].value_0 = (int32_t)(self->temperature.pressure_hPa);
            self->send_msg_buffer[i].value_2 = self->temperature.temperature_C;

            ++self->send_msg_buffer_size;
        }
    }

    return;
}

void node_B02_process_remote_button (node_B02_t * const self,
                                    board_remote_button_t remote_button)
{
    UNUSED(remote_button);

    assert(self != NULL);

    // Do nothing

    return;
}

void node_B02_process_door_movement (node_B02_t * const self,
                                    uint32_t time_ms)
{
    assert(self != NULL);

    node_B02_update_time(self, time_ms);
    const uint32_t light_duration_ms        = time_ms - self->light_start_time_ms;
    const uint32_t intrusion_duration_ms    = time_ms - self->intrusion_start_time_ms;

    if (self->mode == SILENCE_MODE)
    {
        if (light_duration_ms > NODE_B02_LIGHT_DURATION_MS)
        {
            self->light_start_time_ms = time_ms;

            if (self->is_dark == true)
            {
                if (self->send_msg_buffer_size != ARRAY_SIZE(self->send_msg_buffer))
                {
                    const size_t i = self->send_msg_buffer_size;
                    size_t j = 0U;

                    self->send_msg_buffer[i].header.source = self->id;
                    self->send_msg_buffer[i].header.dest_array[j] = NODE_T01;
                    ++j;
                    self->send_msg_buffer[i].header.dest_array_size = j;

                    self->send_msg_buffer[i].cmd_id = SET_LIGHT;
                    self->send_msg_buffer[i].value_0 = (int32_t)(LIGHT_ON);

                    ++self->send_msg_buffer_size;
                }
            }
        }
    }

    else if (self->mode == GUARD_MODE)
    {
        if (intrusion_duration_ms > NODE_B02_INTRUSION_DURATION_MS)
        {
            self->intrusion_start_time_ms   = time_ms;
            self->light_start_time_ms       = time_ms;

            if (self->send_msg_buffer_size != ARRAY_SIZE(self->send_msg_buffer))
            {
                const size_t i = self->send_msg_buffer_size;
                size_t j = 0U;

                self->send_msg_buffer[i].header.source = self->id;
                self->send_msg_buffer[i].header.dest_array[j] = NODE_BROADCAST;
                ++j;
                self->send_msg_buffer[i].header.dest_array_size = j;

                self->send_msg_buffer[i].cmd_id = SET_INTRUSION;
                self->send_msg_buffer[i].value_0 = (int32_t)(INTRUSION_ON);

                ++self->send_msg_buffer_size;
            }
        }
    }

    return;
}

void node_B02_process_front_movement (  node_B02_t * const self,
                                        uint32_t time_ms)
{
    assert(self != NULL);

    node_B02_process_door_movement(self, time_ms);

    return;
}

void node_B02_process_veranda_movement (node_B02_t * const self,
                                        uint32_t time_ms)
{
    assert(self != NULL);

    node_B02_update_time(self, time_ms);
    const uint32_t intrusion_duration_ms    = time_ms - self->intrusion_start_time_ms;
    const uint32_t display_duration_ms      = time_ms - self->display_start_time_ms;

    if (self->mode == SILENCE_MODE)
    {
        if (display_duration_ms > NODE_B02_DISPLAY_DURATION_MS)
        {
            self->display_start_time_ms = time_ms;
        }
    }

    else if (self->mode == GUARD_MODE)
    {
        if (intrusion_duration_ms > NODE_B02_INTRUSION_DURATION_MS)
        {
            self->intrusion_start_time_ms   = time_ms;
            self->light_start_time_ms       = time_ms;

            if (self->send_msg_buffer_size != ARRAY_SIZE(self->send_msg_buffer))
            {
                const size_t i = self->send_msg_buffer_size;
                size_t j = 0U;

                self->send_msg_buffer[i].header.source = self->id;
                self->send_msg_buffer[i].header.dest_array[j] = NODE_BROADCAST;
                ++j;
                self->send_msg_buffer[i].header.dest_array_size = j;

                self->send_msg_buffer[i].cmd_id = SET_INTRUSION;
                self->send_msg_buffer[i].value_0 = (int32_t)(INTRUSION_ON);

                ++self->send_msg_buffer_size;
            }
        }
    }

    return;
}

void node_B02_process_msg ( node_B02_t * const self,
                            node_msg_t const * const rcv_msg,
                            uint32_t time_ms)
{
    assert(self     != NULL);
    assert(rcv_msg  != NULL);

    // Check node destination id
    {
        bool is_dest_node = false;

        for (size_t i = 0U; i < rcv_msg->header.dest_array_size; ++i)
        {
            if ((rcv_msg->header.dest_array[i] == self->id) || (rcv_msg->header.dest_array[i] == NODE_BROADCAST))
            {
                is_dest_node = true;

                break;
            }
        }

        if (is_dest_node == false)
        {
            return;
        }
    }

    node_B02_update_time(self, time_ms);
    const uint32_t light_duration_ms        = time_ms - self->light_start_time_ms;
    const uint32_t intrusion_duration_ms    = time_ms - self->intrusion_start_time_ms;

    if (rcv_msg->cmd_id == SET_MODE)
    {
        self->mode = (node_mode_id_t)(rcv_msg->value_0);

        self->display_start_time_ms     = 0U;
        self->intrusion_start_time_ms   = 0U;
        self->light_start_time_ms       = 0U;
    }

    else if (rcv_msg->cmd_id == SET_INTRUSION) 
    {
        const node_intrusion_id_t intrusion_id = (node_intrusion_id_t)(rcv_msg->value_0);

        if (intrusion_id == INTRUSION_ON)
        {
            if (intrusion_duration_ms > NODE_B02_INTRUSION_DURATION_MS)
            {
                self->intrusion_start_time_ms   = time_ms;
                self->light_start_time_ms       = time_ms;
            }
        }
        else
        {
            self->intrusion_start_time_ms = 0U;
        }
    }

    else if (rcv_msg->cmd_id == SET_LIGHT) 
    {
        const node_light_id_t light_id = (node_light_id_t)(rcv_msg->value_0);

        if (light_id == LIGHT_ON)
        {
            if (light_duration_ms > NODE_B02_LIGHT_DURATION_MS)
            {
                self->light_start_time_ms = time_ms;
            }
        }
        else
        {
            self->light_start_time_ms = 0U;
        }
    }

    return;
}

void node_B02_get_display_data (node_B02_t const * const self,
                                node_B02_temperature_t * const data,
                                uint32_t * const disable_time_ms)
{
    assert(self             != NULL);
    assert(data             != NULL);
    assert(disable_time_ms  != NULL);

    *disable_time_ms = NODE_B02_DISPLAY_DURATION_MS;

    *data = self->temperature;

    return;
}

void node_B02_get_msg ( node_B02_t * const self,
                        node_msg_t *msg,
                        bool * const is_msg_valid)
{
    *is_msg_valid = false;

    if (self->send_msg_buffer_size != 0U)
    {
        *is_msg_valid = true;

        const size_t i = self->send_msg_buffer_size - 1U;

        memcpy((void*)(msg), (const void*)(&self->send_msg_buffer[i]), sizeof(node_msg_t));
        --self->send_msg_buffer_size;
    }

    return;
}
