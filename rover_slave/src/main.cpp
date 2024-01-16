#include <Arduino.h>
#include "messages.hpp"

uint8_t serial_data[256];
uint8_t serial_data_idx;

enum
{
    /* 00 */ E_SERIAL_STATE_WAIT_BODY,
    /* 01 */ E_SERIAL_STATE_WAIT_TYPE,
    /* 02 */ E_SERIAL_STATE_WAIT_TEXT_BODY
};

constexpr uint8_t SERIAL_TEXT_MSG_DELIMITER = ';';

uint32_t serial_idle_count;
uint8_t serial_state;
uint8_t expected_size;

constexpr uint8_t ENABLE_LOG_TRAY = true;
uint8_t *log_tray[256];
uint8_t log_tray_top;
uint8_t log_tray_bot;

uint8_t sprintf_buffer[256];

struct timer_config
{
    uint8_t prescale;
    bool configured;
};

timer_config timers[3];

void setup()
{
    Serial.begin(19200);

    log_tray_top = 0;
    log_tray_bot = 0;

    serial_data_idx = 0;
    serial_idle_count = 0;
    serial_state = E_SERIAL_STATE_WAIT_TYPE;

    memset(&timers, 0, sizeof(timers));
}

void send_serial(uint8_t *data, uint8_t size)
{
    while (size)
    {
        Serial.write(*data);
        data++;
        size--;
    }
}

void send_serial(uint8_t *data)
{
    while (*data)
    {
        Serial.write(*data);
        data++;
    }
}

void initPwm(uint8_t timer_, uint8_t channel, uint8_t prescale)
{
    timer_config& timer = timers[timer_];

    if (0u == timer_)
    {
        if (!timer.configured)
        {
            TCCR0A = _BV(WGM01)  | // FastPWM 0xFF
                     _BV(WGM00);
            TCCR0B = prescale & 0b111;
        }
        else
        {
            if (timer.prescale != prescale)
            {
                TCCR0B = prescale & 0b111;
            }
        }

        uint8_t channel_bits;
        if (!channel)
        {
            // @note  : Clear OC1A on CM
            channel_bits = _BV(COM0A1);
        }
        else
        {
            // @note : Clear OC1B on CM
            channel_bits = _BV(COM0B1);
        }

        TCCR0A |=  channel_bits;

        timer.prescale = prescale;
        timer.configured = true;
    }
    else if (1u == timer_)
    {
        if (!timer.configured)
        {
            TCCR1A = _BV(WGM10); // FastPWM 0xFF
            TCCR1B = _BV(WGM10) | (prescale & 0b111);
        }
        else
        {
            if (timer.prescale != prescale)
            {
                TCCR1B =  (TCCR1B&0b11111000)  | (prescale&0b111);
            }
        }

        uint8_t channel_bits;
        if (!channel)
        {
            // @note  : Clear OC2A on CM
            channel_bits = _BV(COM2A1);
        }
        else
        {
            // @note : Clear OC2B on CM
            channel_bits = _BV(COM2B1);
        }

        TCCR1A |=  channel_bits;

        timer.prescale = prescale;
        timer.configured = true;
    }
    else if (2u == timer_)
    {
        if (!timer.configured)
        {
            TCCR2A = _BV(WGM21)  | // FastPWM 0xFF
                     _BV(WGM20);
            TCCR2B = prescale & 0b111;
        }
        else
        {
            if (timer.prescale != prescale)
            {
                TCCR0B = prescale & 0b111;
            }
        }

        uint8_t channel_bits;
        if (!channel)
        {
            // @note  : Clear OC0A on CM
            channel_bits = _BV(COM2A1);
        }
        else
        {
            // @note : Clear OC0B on CM
            channel_bits = _BV(COM2B1);
        }

        TCCR2A |=  channel_bits;

        timer.prescale = prescale;
        timer.configured = true;
    }
}

void pwmDuty(uint8_t timer, uint8_t channel, uint8_t duty)
{
    if (0u == timer)
    {
        if (!channel)
            OCR0A = duty;
        else
            OCR0B = duty;
    }
    else if (1u == timer)
    {
        if (!channel)
            OCR1A = duty;
        else
            OCR1B = duty;
    }
    else if (2u == timer)
    {
        if (!channel)
            OCR2A = duty;
        else
            OCR2B = duty;
    }
}

void process_message(const uint8_t* serial_data)
{
    auto type = *serial_data;
    switch (type)
    {
        case messages::E_TYPE_MSGB_LOG_INFO_REQUEST:
        {
            // auto msg = (messages::msgb_log_info_request*)(serial_data);
            break;
        }
        case messages::E_TYPE_MSGB_PINMODE_REQUEST:
        {
            auto msg = (messages::msgb_pinmode_request*)(serial_data);
            auto pin = messages::get_param<uint8_t>(msg->mode_pin, msg->MASK_PIN,  msg->SHIFT_PIN);
            auto mode = messages::get_param<uint8_t>(msg->mode_pin, msg->MASK_MODE, msg->SHIFT_MODE);

            pinMode(pin, mode);
            messages::msgb_pinmode_response rsp{};
            send_serial((uint8_t*)&rsp, sizeof(rsp));
            break;
        }
        case messages::E_TYPE_MSGB_DIGITAL_WRITE_REQUEST:
        {
            auto msg = (messages::msgb_digital_write_request*)(serial_data);
            digitalWrite(
                messages::get_param<uint8_t>(msg->value_pin, msg->MASK_PIN,   msg->SHIFT_PIN),
                messages::get_param<uint8_t>(msg->value_pin, msg->MASK_VALUE, msg->SHIFT_VALUE));
            messages::msgb_digital_write_response rsp{};
            send_serial((uint8_t*)&rsp, sizeof(rsp));
            break;
        }
        case messages::E_TYPE_MSGB_DIGITAL_READ_REQUEST:
        {
            auto msg = (messages::msgb_digital_read_request*)(serial_data);
            messages::msgb_digital_read_response rsp{};
            rsp.rval = digitalRead(msg->pin);
            send_serial((uint8_t*)&rsp, sizeof(rsp));
            break;
        }
        case messages::E_TYPE_MSGB_ANALOG_READ_REQUEST:
        {
            auto msg = (messages::msgb_analog_read_request*)(serial_data);
            messages::msgb_analog_read_response rsp{};
            rsp.rval = analogRead(msg->pin);
            send_serial((uint8_t*)&rsp, sizeof(rsp));
            break;
        }
        case messages::E_TYPE_MSGB_PWM_INIT_REQUEST:
        {
            auto msg = (messages::msgb_pwm_init_request*)(serial_data);
            initPwm(
                messages::get_param<uint8_t>(msg->prescaler_timer_channel, msg->MASK_TIMER, msg->SHIFT_TIMER),
                messages::get_param<uint8_t>(msg->prescaler_timer_channel, msg->MASK_CHANNEL, msg->SHIFT_CHANNEL),
                messages::get_param<uint8_t>(msg->prescaler_timer_channel, msg->MASK_PRESCALER, msg->SHIFT_PRESCALER)
            );
            messages::msgb_pwm_init_response rsp{};
            send_serial((uint8_t*)&rsp, sizeof(rsp));
            break;
        }
        case messages::E_TYPE_MSGB_PWM_DUTY_REQUEST:
        {
            auto msg = (messages::msgb_pwm_duty_request*)(serial_data);
            pwmDuty(
                messages::get_param<uint8_t>(msg->timer_channel, msg->MASK_TIMER, msg->SHIFT_TIMER),
                messages::get_param<uint8_t>(msg->timer_channel, msg->MASK_CHANNEL, msg->SHIFT_CHANNEL),
                msg->duty
            );
            messages::msgb_pwm_duty_response rsp{};
            send_serial((uint8_t*)&rsp, sizeof(rsp));
            break;
        }
    }
}

void process_text_message(const uint8_t* serial_data)
{
    auto type = *serial_data;
    serial_data++;

    switch(type)
    {
        case 'A': // type  0 msg_log_info_request
        {
            Serial.write(';');
            break;
        }
        case 'B': // type  1 msgb_pinmode_request(u2 pin, u2 mode)
        {
            auto pin  = messages::get_str_param_n<uint8_t>(serial_data, 2);
            serial_data += 2;

            auto mode = messages::get_str_param_n<uint8_t>(serial_data, 2);
            serial_data += 2;

            pinMode(pin, mode);

            sprintf((char*)sprintf_buffer, "pinMode(%u,%u);", pin, mode);

            send_serial(sprintf_buffer);
            break;
        }
        case 'C': // type  2 msgb_digital_write_request(u2 pin, u2 value)
        {
            auto pin  = messages::get_str_param_n<uint8_t>(serial_data, 2);
            serial_data += 2;

            auto value = messages::get_str_param_n<uint8_t>(serial_data, 2);
            serial_data += 2;

            digitalWrite(pin, value);

            sprintf((char*)sprintf_buffer, "digitalWrite(%u,%u);", pin, value);

            send_serial(sprintf_buffer);
            break;
        }
        case 'D': // type  3 msgb_digital_read_request
        {
            auto pin  = messages::get_str_param_n<uint8_t>(serial_data, 2);

            uint8_t value = digitalRead(pin);

            sprintf((char*)sprintf_buffer, "%u=digitalRead(%u);", value, pin);

            send_serial(sprintf_buffer);
            break;
        }
        case 'E': // type  4 msgb_analog_read_request
        {
            auto pin  = messages::get_str_param_n<uint8_t>(serial_data, 2);

            uint16_t value = analogRead(pin);

            sprintf((char*)sprintf_buffer, "%u=analogRead(%u);", value, pin);
            send_serial(sprintf_buffer);

            break;
        }
        case 'F': // type  5 msgb_pwm_init_request(u1 timer, u1 channel, u1 prescale)
        {
            auto timer    = messages::get_str_param_n<uint8_t>(serial_data, 1);
            serial_data++;

            auto channel  = messages::get_str_param_n<uint8_t>(serial_data, 1);
            serial_data++;

            auto prescale = messages::get_str_param_n<uint8_t>(serial_data, 1);

            initPwm(timer, channel, prescale);

            sprintf((char*)sprintf_buffer, "initPwm(%u,%u,%u);", timer, channel, prescale);
            send_serial(sprintf_buffer);

            break;
        }
        case 'G': // type  6 msgb_pwm_duty_request(u1 timer, u1 channel,u3 duty)
        {
            auto timer    = messages::get_str_param_n<uint8_t>(serial_data, 1);
            serial_data++;

            auto channel  = messages::get_str_param_n<uint8_t>(serial_data, 1);
            serial_data++;

            auto duty = messages::get_str_param_n<uint8_t>(serial_data, 3);

            pwmDuty(timer, channel, duty);

            sprintf((char*)sprintf_buffer, "pwmDuty(%u,%u,%u);", timer, channel, duty);
            send_serial(sprintf_buffer);

            break;
        }
    }
}

void loop()
{
    while (Serial.available())
    {
        serial_idle_count = 0;

        auto rb = Serial.read();
        serial_data[serial_data_idx++] = rb;

        // Serial.print("in_data");
        // Serial.print("@");
        // Serial.print(serial_data_idx-1);
        // Serial.print(" s(");
        // Serial.print(serial_state);
        // Serial.print("):");
        // Serial.print(char(rb));
        // Serial.print("(");
        // Serial.print(rb);
        // Serial.print(")\n");

        if (E_SERIAL_STATE_WAIT_TYPE == serial_state)
        {
            // @note : binary message
            if (uint8_t(rb) < sizeof(messages::msg_size_by_type))
            {
                expected_size = messages::msg_size_by_type[rb] - 1;

                // @note : header only message
                if (!expected_size)
                {
                    process_message(serial_data);
                    serial_data_idx = 0;
                    continue;
                }

                serial_state = E_SERIAL_STATE_WAIT_BODY;
            }
            // @note : text message
            else
            {
                serial_state = E_SERIAL_STATE_WAIT_TEXT_BODY;
            }
            continue;
        }

        if (E_SERIAL_STATE_WAIT_BODY == serial_state)
        {
            expected_size--;
            if (!expected_size)
            {
                process_message(serial_data);
                serial_state = E_SERIAL_STATE_WAIT_TYPE;
                serial_data_idx = 0;
            }
            continue;
        }

        if (E_SERIAL_STATE_WAIT_TEXT_BODY == serial_state)
        {
            if (SERIAL_TEXT_MSG_DELIMITER == rb)
            {
                serial_data[serial_data_idx++] = 0;

                process_text_message(serial_data);
                serial_state = E_SERIAL_STATE_WAIT_TYPE;
                serial_data_idx = 0;
            }
            continue;
        }
    }

    serial_idle_count++;

    // serial_idle_count 136.79 = 1ms
    // serial_idle_count 273.58 = 2ms
    // serial_idle_count 683.95 = 5ms

    // @note : 2ms
    if (serial_idle_count > 274u)
    {
        serial_idle_count = 0;
        serial_state = E_SERIAL_STATE_WAIT_TYPE;
        serial_data_idx = 0;
    }
}
