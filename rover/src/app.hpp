#ifndef __APP_HPP__
#define __APP_HPP__

#include <bfc/Udp.hpp>
#include <bfc/Buffer.hpp>

#include "arduino.hpp"
#include "filter.hpp"

#include <map>
#include <deque>
#include <string>
#include <thread>
#include <variant>
#include <fstream>
#include <vector>
#include <mutex>

namespace app
{

using options_t = std::map<std::string, std::string>;

struct js_data_t
{
    int left_x;
    int right_x;
    int left_y;
    int right_y;
};

struct queue_msg_t
{
    enum type_e {JS, TTY, TIMER};
    type_e type;
    union value_u
    {
        uint64_t timer;
        js_data_t* js_data;
        char* str_data;
    };

    value_u value;
};

class main_app
{
public:
    main_app(options_t& options);
    int run();
private:

    void run_udp();
    void run_timer();
    void run_ws();

    void handle_udp(bfc::ConstBufferView);
    void handle_js(const js_data_t&);
    void handle_timer(uint64_t);
    void handle_cmd(const char*);
    void handle_ws(uint32_t, const std::string&);

    // @note js udp rcv
    bfc::UdpSocket js_sock;
    std::thread udp_thread;

    // @note ws rcv
    std::thread ws_thread;

    // @note internal queue
    enum {QUEUE_SOCK_IN, QUEUE_SOCK_OUT};
    int queue_socks[2];

    // @note counters
    std::thread timer_thread;
    const unsigned TIMER_PERIOD_US = 1000;    // 1000us
    const unsigned TIMER_CI_WDT = 500;        // 1000us * 500
    const unsigned TIMER_CI_FAN = 1000;       // 1000us * 1000
    const unsigned TIMER_CI_THR = 10;         // 1000us * 10
    const unsigned TIMER_CI_THR_PRNT = 1000;  // 1000us * 1000
    const unsigned TIMER_CI_BATT = 1000;      // 1000us * 1000
    const unsigned TIMER_CI_SLAVE_TRX = 1000; // 1000us * 1000
    const unsigned TIMER_CI_WLAN = 100; // 1000us * 1000
    unsigned timer_cx_wdt = 0;
    unsigned timer_cx_fan = 0;
    unsigned timer_cx_thr = 0;
    unsigned timer_cx_thr_prnt = 0;
    unsigned timer_cx_batt = 0;
    unsigned timer_cx_slave_trx = 0;
    unsigned timer_cx_wlan = 0;

    // @note thermal_limiter
    unsigned PWM_RANGE = 255;
    double PWM_POWER = 0.0012;
    double HEAT_TRANSFER_KA = 0.000001;
    double SPECIFIC_HEAT  = 0.466;
    double MASS  = 0.06;
    unsigned left_pwm = 0;
    unsigned right_pwm = 0;
    unsigned fan_pwm = 0;
    unsigned led_pwm = 0;
    double heat = 0;
    double thr_limit = 1;
    double ambient = 25;
    double mot_temp = 0;
    double soc_temp = 0;
    double vbat;
    double vbat_slope;
    int wifi_quality = 0;
    int wifi_signal = 0;
    std::pair<unsigned,unsigned> slave_trx = {0,0};

    std::ofstream vbat_csv;
    int64_t vbat_s0 = 0;
    int VBAT_FILTER_SIZE = 5;
    dsp::averaging_filter vbatfb;
    dsp::averaging_filter vbatfb2;

    options_t& options;
    arduino::ArduinoSlave slave;

};

} // namespace app

#endif // __APP_HPP__
