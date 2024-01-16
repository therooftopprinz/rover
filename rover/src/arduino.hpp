#ifndef __ARDUINO_HPP__
#define __ARDUINO_HPP__

#include <wiringSerial.h>
#include <string>
#include <mutex>
#include <map>

namespace arduino
{

using options_t = std::map<std::string, std::string>;

class ArduinoSlave
{
public:
    ArduinoSlave(options_t& options);
    void pinMode(uint8_t pin, uint8_t mode);
    void initPwm(uint8_t timer, uint8_t channel, uint8_t prescale);
    void pwmDuty(uint8_t timer, uint8_t channel, uint8_t duty);
    uint16_t analogRead(uint8_t pin);

    std::pair<unsigned, unsigned> get_transaction_stats();
private:
    void send(uint8_t*, size_t);
    bool recv(uint8_t*, size_t);
    int serial_fd;
    int max_retry;
    unsigned transaction_sent = 0;
    unsigned transaction_received = 0;
    std::mutex access;
};

} // namespace arduino

#endif // __ARDUINO_HPP__
