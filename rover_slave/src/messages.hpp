#pragma once

#include <stdint.h>

namespace messages
{

template <typename T, typename U, typename V>
void set_param(T value, U& field, V mask, uint8_t shift)
{
    field &= ~mask;
    field |= (value << shift);
}

template <typename T, typename U, typename V>
T get_param(U field, V mask, uint8_t shift)
{
    return (field&mask) >> shift;
}
enum
{
/* 00 */   E_TYPE_MSGB_LOG_INFO_REQUEST,
/* 01 */   E_TYPE_MSGB_PINMODE_REQUEST,
/* 02 */   E_TYPE_MSGB_DIGITAL_WRITE_REQUEST,
/* 03 */   E_TYPE_MSGB_DIGITAL_READ_REQUEST,
/* 04 */   E_TYPE_MSGB_ANALOG_READ_REQUEST,
/* 05 */   E_TYPE_MSGB_PWM_INIT_REQUEST,
/* 06 */   E_TYPE_MSGB_PWM_DUTY_REQUEST
};


struct __attribute__((packed)) msgb_log_info_request
{
    uint8_t type = 0;
};

struct __attribute__((packed)) msgb_log_info_response
{
    uint8_t size;
};

struct __attribute__((packed)) msgb_pinmode_request
{
    uint8_t type = 1;
    uint8_t mode_pin;
    constexpr static uint8_t MASK_MODE = 0b11000000;
    constexpr static uint8_t MASK_PIN  = 0b00111111;
    constexpr static uint8_t SHIFT_MODE = 6;
    constexpr static uint8_t SHIFT_PIN  = 0;
};

struct __attribute__((packed)) msgb_pinmode_response
{
    uint8_t rval = 0;
};

struct __attribute__((packed)) msgb_digital_write_request
{
    uint8_t type = 2;
    uint8_t value_pin;
    constexpr static uint8_t MASK_VALUE = 0b11000000;
    constexpr static uint8_t MASK_PIN  =  0b00111111;
    constexpr static uint8_t SHIFT_VALUE = 6;
    constexpr static uint8_t SHIFT_PIN  = 0;
};

struct __attribute__((packed)) msgb_digital_write_response
{
    uint8_t rval = 0;
};

struct __attribute__((packed)) msgb_digital_read_request
{
    uint8_t type = 3;
    uint8_t pin;
};

struct __attribute__((packed)) msgb_digital_read_response
{
    uint8_t rval = 0;
};

struct __attribute__((packed)) msgb_analog_read_request
{
    uint8_t type = 4;
    uint8_t pin;
};

struct __attribute__((packed)) msgb_analog_read_response
{
    uint16_t rval = 0;
};

struct __attribute__((packed)) msgb_pwm_init_request
{
    uint8_t type = 5;
    uint8_t prescaler_timer_channel = 0;
    constexpr static uint8_t MASK_PRESCALER = 0b00111000;
    constexpr static uint8_t MASK_TIMER     = 0b00000110;
    constexpr static uint8_t MASK_CHANNEL   = 0b00000001;
    constexpr static uint8_t SHIFT_PRESCALER = 3;
    constexpr static uint8_t SHIFT_TIMER     = 1;
    constexpr static uint8_t SHIFT_CHANNEL   = 0;
};

struct __attribute__((packed)) msgb_pwm_init_response
{
    uint8_t rval = 0;
};

struct __attribute__((packed)) msgb_pwm_duty_request
{
    uint8_t type = 6;
    uint8_t timer_channel;
    uint8_t duty;
    constexpr static uint8_t MASK_TIMER     = 0b00000110;
    constexpr static uint8_t MASK_CHANNEL   = 0b00000001;
    constexpr static uint8_t SHIFT_TIMER     = 1;
    constexpr static uint8_t SHIFT_CHANNEL   = 0;
};

struct __attribute__((packed)) msgb_pwm_duty_response
{
    uint8_t rval = 0;
};

extern uint8_t msg_size_by_type[7];

template <typename T>
T  get_str_param_n (const uint8_t* cdata, uint8_t size)
{
    T rval = 0;
    for (uint8_t i = 0u; i<size; i++)
    {
        auto digit = cdata[i]-'0';
        rval += digit;
        rval *= pow(10, size-i-1);
    }
    return rval;
}

} // namespace messages