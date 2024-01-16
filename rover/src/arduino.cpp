#include "arduino.hpp"
#include "messages.hpp"
#include "Log.hpp"

#include <sstream>
#include <cstring>
#include <iomanip>

namespace arduino
{

ArduinoSlave::ArduinoSlave(options_t& options)
    : max_retry(5)
{
    std::string sdev;
    uint32_t baud = 19200;
    if (options.count("sdev"))
    {
        sdev = options["sdev"];
    }
    else
    {
        sdev = "/dev/serial0";
    }

    if (options.count("baud"))
    {
        baud = std::stoul(options["baud"]);
    }

    serial_fd = serialOpen (sdev.c_str(), baud);

    LOG("ArduinoSlave::ArduinoSlave | Opening serial %s at %d.\n", sdev.c_str(), baud);

    if (serial_fd < 0)
    {
        std::stringstream ss;
        ss << "Failed to open serial device: ";
        ss << strerror(errno);
        throw std::runtime_error(ss.str());
    }
}

void ArduinoSlave::send(uint8_t* cdata, size_t size)
{
    transaction_sent++;
    serialFlush(serial_fd);
    std::stringstream ss;
    ss << "data={";
    for (size_t i=0; i<size; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << int(cdata[i]);
        serialPutchar(serial_fd, cdata[i]);
    }
    ss << "}";

    // LOG("ArduinoSlave::send | %s\n", ss.str().c_str());
}

bool ArduinoSlave::recv(uint8_t* data, size_t size)
{
    uint32_t idle_counter = 0;
    std::stringstream ss;
    ss << "data[expected=" << size << "]{";

    while (size)
    {
        while(serialDataAvail(serial_fd) && size)
        {
            *data = serialGetchar(serial_fd);
            ss << std::hex << std::setw(2) << std::setfill('0') << int(*data);
            size--;
            data++;
        }

        if (!size)
        {
            ss << "}";
            // LOG("ArduinoSlave::recv | OK %s\n", ss.str().c_str());
            transaction_received++;
            return true;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (idle_counter > 50)
        {
            ss << "}";
            LOG("ArduinoSlave::recv | NOK %s\n", ss.str().c_str());
            return false;
        }
        idle_counter++;
    }
    return true;
}

void ArduinoSlave::pinMode(uint8_t pin, uint8_t mode)
{
    std::unique_lock<std::mutex> lg(access);

    messages::msgb_pinmode_request msg{};
    msg.type = messages::E_TYPE_MSGB_PINMODE_REQUEST;
    messages::set_param(pin,  msg.mode_pin,  msg.MASK_PIN,  msg.SHIFT_PIN);
    messages::set_param(mode, msg.mode_pin, msg.MASK_MODE, msg.SHIFT_MODE);
    for (int i = 0; i<max_retry; i++)
    {
        messages::msgb_pinmode_response rsp{};
        send((uint8_t*)(&msg), sizeof(msg));
        if (recv((uint8_t*)(&msg), sizeof(rsp)))
        {
            break;
        }
    }
}

void ArduinoSlave::initPwm(uint8_t timer, uint8_t channel, uint8_t prescale)
{
    std::unique_lock<std::mutex> lg(access);

    messages::msgb_pwm_init_request msg{};
    msg.type = messages::E_TYPE_MSGB_PWM_INIT_REQUEST;
    messages::set_param(timer,    msg.prescaler_timer_channel,  msg.MASK_TIMER,   msg.SHIFT_TIMER);
    messages::set_param(channel,  msg.prescaler_timer_channel,  msg.MASK_CHANNEL, msg.SHIFT_CHANNEL);
    messages::set_param(prescale, msg.prescaler_timer_channel,  msg.MASK_PRESCALER, msg.SHIFT_PRESCALER);
    for (int i = 0; i<max_retry; i++)
    {
        messages::msgb_pwm_init_response rsp{};
        send((uint8_t*)(&msg), sizeof(msg));
        if (recv((uint8_t*)(&msg), sizeof(rsp)))
        {
            break;
        }
    }
}

void ArduinoSlave::pwmDuty(uint8_t timer, uint8_t channel, uint8_t duty)
{
    std::unique_lock<std::mutex> lg(access);

    messages::msgb_pwm_duty_request msg{};
    msg.type = messages::E_TYPE_MSGB_PWM_DUTY_REQUEST;
    messages::set_param(timer,    msg.timer_channel,  msg.MASK_TIMER,   msg.SHIFT_TIMER);
    messages::set_param(channel,  msg.timer_channel,  msg.MASK_CHANNEL, msg.SHIFT_CHANNEL);
    msg.duty = duty;
    for (int i = 0; i<max_retry; i++)
    {
        messages::msgb_pwm_duty_response rsp{};
        send((uint8_t*)(&msg), sizeof(msg));
        if (recv((uint8_t*)(&msg), sizeof(rsp)))
        {
            break;
        }
    }
}

uint16_t ArduinoSlave::analogRead(uint8_t pin)
{
    std::unique_lock<std::mutex> lg(access);

    messages::msgb_analog_read_request msg{};
    msg.type = messages::E_TYPE_MSGB_ANALOG_READ_REQUEST;
    msg.pin = pin;
    for (int i = 0; i<max_retry; i++)
    {
        messages::msgb_analog_read_response rsp{};
        send((uint8_t*)(&msg), sizeof(msg));
        if (recv((uint8_t*)(&rsp), sizeof(rsp)))
        {
            return rsp.rval;
        }
    }
    return -1;
}

std::pair<unsigned, unsigned> ArduinoSlave::get_transaction_stats()
{
    std::unique_lock<std::mutex> lg(access);

    auto tx = transaction_sent;
    auto rx = transaction_received;
    transaction_sent = 0;
    transaction_received = 0;

    return {rx,tx};
}

} // namespace arduino
