#include "app.hpp"
#include "Log.hpp"
#include "ws_server_isolated.hpp"

#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <cmath>
#include <numeric>
#include <regex>

#include <wiringPi.h>
#include <softPwm.h>

using namespace app;

// INPUT A B C D L
// PINS  3 5 6 9 10
// TIMER 2 0 0 1  1
// CHAN  1 1 0 0  1

constexpr int PIN_HBRIDGE_A = 3;
constexpr int PIN_HBRIDGE_B = 5;
constexpr int PIN_HBRIDGE_C = 6;
constexpr int PIN_HBRIDGE_D = 9;
// constexpr int PIN_FAN       = 6;
constexpr int PIN_LED       = 10;
constexpr int PIN_BATT      = 14;

constexpr double R_BATT_TOP      = 47000;
constexpr double R_BATT_BOT      = 10000;
constexpr double V_REF           = 3.33;

main_app::main_app(options_t& options)
    : options(options)
    , vbatfb(VBAT_FILTER_SIZE)
    , vbatfb2(VBAT_FILTER_SIZE*50)
    , slave(options)
{
    uint16_t udp_bind_port;
    if (options.count("js_port"))
    {
        udp_bind_port = std::stoul(options["js_port"]);
    }
    else
    {
        udp_bind_port = 11223;
    }

    if (options.count("pwmr"))
    {
        PWM_RANGE = std::stoul(options["pwmr"]);
    }

    if (options.count("ka"))
    {
        HEAT_TRANSFER_KA = std::stof(options["ka"]);
    }

    if (options.count("c"))
    {
        SPECIFIC_HEAT = std::stof(options["c"]);
    }

    if (options.count("m"))
    {
        MASS = std::stof(options["m"]);
    }

    if (options.count("p"))
    {
        PWM_POWER = std::stof(options["p"]);
    }

    LOG("main_app::main_app | Listening to port %d.\n", udp_bind_port);

    js_sock.bind(bfc::IpPort(0, udp_bind_port));

    if (0 > socketpair(AF_UNIX, SOCK_DGRAM, 0, queue_socks))
    {
        std::stringstream ss;
        ss << "Failed to create socketpair! Error=";
        ss << strerror(errno);
        throw std::runtime_error(ss.str());
    }

    std::string vbat_csv_fn = "vbat.csv";
    if (options.count("vbat_csv"))
    {
        PWM_POWER = std::stof(options["vbat_csv"]);
    }

    vbat_csv.open(vbat_csv_fn, std::ios_base::app);

    udp_thread = std::thread([this](){run_udp();});
    timer_thread = std::thread([this](){run_timer();});
    ws_thread = std::thread([this](){run_ws();});

    slave.pinMode(PIN_HBRIDGE_A , OUTPUT);
    slave.pinMode(PIN_HBRIDGE_B , OUTPUT); 
    slave.pinMode(PIN_HBRIDGE_C , OUTPUT);
    slave.pinMode(PIN_HBRIDGE_D , OUTPUT);
    // slave.pinMode(PIN_FAN ,       OUTPUT);
    slave.pinMode(PIN_LED ,       OUTPUT);

    slave.initPwm(2,1,1); // HBRIDGE_A
    slave.initPwm(0,1,1); // HBRIDGE_B
    slave.initPwm(0,0,1); // HBRIDGE_C
    slave.initPwm(1,0,1); // HBRIDGE_D
    slave.initPwm(1,1,1); // LED
}


void main_app::run_udp()
{
    std::byte rcv_raw_buffer[1024];
    auto rcv_buffer = bfc::BufferView(rcv_raw_buffer, sizeof(rcv_raw_buffer));
    bfc::IpPort sender;
    while (true)
    {
        js_sock.recvfrom(rcv_buffer,sender);
        handle_udp(rcv_buffer);
    }
}

void main_app::run_timer()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(TIMER_PERIOD_US));
        queue_msg_t msg;
        msg.type = queue_msg_t::TIMER;
        msg.value.timer = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        send(queue_socks[QUEUE_SOCK_OUT], &msg, sizeof(msg), 0);
    }
}

void main_app::run_ws()
{
    uint8_t queue_msg_buff[64];
    while (true)
    {
        auto rb = recv(ws::ws_server_queue_socks[ws::QUEUE_SOCK_USER], queue_msg_buff, sizeof(queue_msg_buff), 0);
        if (0 > rb)
        {
            continue;
        }

        ws::queue_msg_t& msg = *((ws::queue_msg_t*)queue_msg_buff);
        std::string msg_str(msg.data_str);
        delete[] msg.data_str;
        handle_ws(msg.conn_id, msg_str);
    }
}

int main_app::run()
{
    uint8_t queue_msg_buff[64];
    while (true)
    {
        auto rb = recv(queue_socks[QUEUE_SOCK_IN], queue_msg_buff, sizeof(queue_msg_buff), 0);
        queue_msg_t& msg = *((queue_msg_t*)queue_msg_buff);
        switch (msg.type)
        {
            case queue_msg_t::JS:
                handle_js(*(msg.value.js_data));
                delete msg.value.js_data;
                break;
            case queue_msg_t::TIMER:
                handle_timer(msg.value.timer);
                break;
        }
    }
    return 0;
}

void main_app::handle_cmd(const char* cmdline)
{
    std::stringstream ss(cmdline);
    std::string cmd;
    ss >> cmd;
    if (cmd=="led")
    {
        unsigned pwm;
        ss >> pwm;
        led_pwm = pwm;
        slave.pwmDuty(1, 1, led_pwm*255/100);
    }
}

void main_app::handle_udp(bfc::ConstBufferView buffer)
{
    const char* data = (const char*)buffer.data();

    if (*data < '0' || *data > '9')
    {
        handle_cmd(data);
        return;
    }

    auto js = new js_data_t{};
    unsigned left_x  = 0;
    unsigned left_y  = 0;
    unsigned right_x = 0;
    unsigned right_y = 0;

    left_x  += (data[0]-'0')*1000;
    left_x  += (data[1]-'0')*100;
    left_x  += (data[2]-'0')*10;
    left_x  += (data[3]-'0')*1;
    left_y  += (data[4]-'0')*1000;
    left_y  += (data[5]-'0')*100;
    left_y  += (data[6]-'0')*10;
    left_y  += (data[7]-'0')*1;
    right_x += (data[8+0]-'0')*1000;
    right_x += (data[8+1]-'0')*100;
    right_x += (data[8+2]-'0')*10;
    right_x += (data[8+3]-'0')*1;
    right_y += (data[8+4]-'0')*1000;
    right_y += (data[8+5]-'0')*100;
    right_y += (data[8+6]-'0')*10;
    right_y += (data[8+7]-'0')*1;

    js->left_x = left_x-1500;
    js->left_y = left_y-1500;
    js->right_x = right_x-1500;
    js->right_y = right_y-1500;

    queue_msg_t msg;
    msg.type = queue_msg_t::JS;
    msg.value.js_data = js;

    send(queue_socks[QUEUE_SOCK_OUT], &msg, sizeof(msg), 0);
}

void main_app::handle_js(const js_data_t& js_data)
{
    // @note reset wdt
    timer_cx_wdt = 0;

    LOG("main_app::handle_js | JS DATA : (%4d, %4d) (%4d, %4d)\n", js_data.left_x, js_data.left_y, js_data.right_x, js_data.right_y);


    double left_y  = (js_data.left_y ) * 1.0 / 500;
    double right_y = (js_data.right_y) * 1.0 / 500;

    left_pwm  = std::abs(left_y) * PWM_RANGE * thr_limit;
    right_pwm = std::abs(right_y)* PWM_RANGE * thr_limit;

    bool left_is_fwd  = left_y > 0;
    bool right_is_fwd = right_y > 0;

    /***
     *  MAPPINGS:
     *    LEFT:
     *      AB
     *      00 STP gpio write 25 0 && gpio write 24 0
     *      01 BWD gpio write 25 0 && gpio write 24 1
     *      10 FWD gpio write 25 1 && gpio write 24 0
     *      11 STP
     * 
     *  constexpr int PIN_HBRIDGE_A = 25;
     *  constexpr int PIN_HBRIDGE_B = 24;
     *  constexpr int PIN_HBRIDGE_C = 29;
     *  constexpr int PIN_HBRIDGE_D = 28;
     *    RIGHT:
     *      CD
     *      00 STP
     *      01 FWD
     *      10 BWD
     *      11 STP
    ***/

    // LEFT
    // LOG("main_app::handle_js | LEFT %s %lf:\n", (left_is_fwd ? "FWD" : "BWD"), left_y);
    if (left_is_fwd)
    {
        slave.pwmDuty(2, 1, left_pwm);
        slave.pwmDuty(0, 1, 0);
    }
    else
    {
        slave.pwmDuty(2, 1, 0);
        slave.pwmDuty(0, 1, left_pwm);
    }
    // RIGHT
    // LOG("main_app::handle_js | RIGHT %s %lf:\n",  (right_is_fwd ? "FWD" : "BWD"), right_y);
    if (right_is_fwd)
    {
        slave.pwmDuty(0, 0, 0);
        slave.pwmDuty(1, 0, right_pwm);
    }
    else
    {
        slave.pwmDuty(0, 0, right_pwm);
        slave.pwmDuty(1, 0, 0);
    }
}

void main_app::handle_timer(uint64_t time)
{
    timer_cx_wdt++;
    if (TIMER_CI_WDT <= timer_cx_wdt)
    {
        LOG("main_app::handle_timer | wdt triggered stop\n");
        slave.pwmDuty(2, 1, 0);
        slave.pwmDuty(0, 1, 0);
        slave.pwmDuty(0, 0, 0);
        slave.pwmDuty(1, 0, 0);

        timer_cx_wdt = 0;
    }

    timer_cx_thr++;
    if (TIMER_CI_THR <= timer_cx_thr)
    {
        double dt = (TIMER_CI_THR*(TIMER_PERIOD_US/1000)*1.0)/1000;
        // heat added to system
        double pwm_power = PWM_POWER * (left_pwm + right_pwm) / (2*PWM_RANGE);
        double heat_in = dt *pwm_power *1.0;
        heat += heat_in;
        // heat removed from system
        double temp = ambient + heat / (SPECIFIC_HEAT * MASS);
        mot_temp = temp;
        double heat_out = HEAT_TRANSFER_KA * (temp-ambient);
        heat -= heat_out;
        if (TIMER_CI_THR_PRNT <= timer_cx_thr_prnt)
        {
            LOG("main_app::handle_timer | heat: %lf heat_in: %lf heat_out: %lf temp: %lf\n", heat, heat_in, heat_out, temp);
            timer_cx_thr_prnt = 0;
        }
        timer_cx_thr_prnt++;
    }

    timer_cx_fan++;
    if (TIMER_CI_FAN <= timer_cx_fan)
    {
        timer_cx_fan = 0;
        std::fstream fs("/sys/class/thermal/thermal_zone0/temp", std::fstream::in);
        int32_t temp_raw;
        fs >> temp_raw;
        float temp = temp_raw*1.0/1000;
        soc_temp = temp;

        if (temp > 0)
        {
            int pwm = (std::round(temp)-40)*2.5;
            pwm = pwm > 0 ? pwm : 0;
            pwm = pwm < 100 ? pwm : 100;
            fan_pwm = pwm;
 
            LOG("main_app::handle_timer | fan temp %f pwm %u\n", temp, pwm);
            // softPwmWrite(PIN_FAN, pwm);
        }
    }

    timer_cx_batt++;
    if (TIMER_CI_BATT <= timer_cx_batt)
    {
        uint16_t raw_batt = slave.analogRead(PIN_BATT);
        if (raw_batt != 0xFFFF)
        {
            timer_cx_batt = 0;
            auto vbat_raw = ((R_BATT_BOT+R_BATT_TOP)/R_BATT_BOT)*V_REF*raw_batt/1024;
            auto s_epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

            auto vbat_presmooth = vbatfb(vbat_raw);
            auto vbat_smooth = vbatfb2(vbat_raw);

            vbat = vbat_presmooth;
            vbat_slope = 1000*(vbatfb2.last(0)-vbatfb2.last(-60))/10;

            if (vbat_s0 != s_epoch)
            {
                vbat_s0 = s_epoch;
                vbat_csv << s_epoch << "," << vbat_raw << "," << vbat_smooth << std::endl;
            }
        }
    }

    timer_cx_slave_trx++;
    if (TIMER_CI_SLAVE_TRX <= timer_cx_slave_trx)
    {
        timer_cx_slave_trx = 0;
        slave_trx = slave.get_transaction_stats();
    }

    timer_cx_wlan++;
    if (TIMER_CI_WLAN <= timer_cx_wlan)
    {
        timer_cx_wlan = 0;
        std::fstream fs("/proc/net/wireless", std::fstream::in);
        std::string line;
        int line_n = 0;
        while (std::getline(fs, line))
        {
            if (line_n==2)
            {
                const std::regex pattern("([^ ]+)[ ]+([^ ]+)[ ]+([^ ]+)[ ]+([^ ]+)[ ]+([^ ]+)[ ]+([^ ]+)[ ]+([^ ]+)[ ]+([^ ]+)[ ]+([^ ]+)[ ]+([^ ]+)[ ]+([^ ]+)[ ]*");
                std::smatch base_match;
                
                // std::string line = " wlan0: 0000   58.  -52.  -256        0      0      0    321      0        0\n";

                std::vector<std::string> cols;

                if (std::regex_search(line, base_match, pattern)) {
                    for (unsigned i = 0; i<base_match.size(); i++)
                    {
                        std::ssub_match base_sub_match = base_match[i];
                        cols.push_back(base_sub_match.str());
                    }

                    if (cols.size()>=4)
                    {
                        wifi_quality = std::stoi(cols[3]);
                        wifi_signal = std::stoi(cols[4]);
                    }
                }
            }
            line_n++;
        }
    }
}

void main_app::handle_ws(uint32_t conn_id, const std::string& str)
{
    std::stringstream ss(str);
    std::string cmd;
    ss >> cmd;

    if (cmd == "stats")
    {
        std::stringstream oss;
        oss << "stats ";
        oss << soc_temp << " ";
        oss << fan_pwm << " ";
        oss << led_pwm << " ";
        oss << mot_temp << " ";
        oss << left_pwm << " ";
        oss << right_pwm << " ";
        oss << vbat << " ";
        oss << vbat_slope << " ";
        oss << slave_trx.first << " ";
        oss << slave_trx.second << " ";
        oss << wifi_quality << " ";
        oss << wifi_signal << "\n";
        ws::ws_send(conn_id, oss.str());
    }
    else if (cmd == "js")
    {
        int left;
        int right;
        ss >> left;
        ss >> right;

        auto js = new js_data_t{};

        js->left_x = 0;
        js->left_y = left*5;
        js->right_x = 0;
        js->right_y = right*5;

        queue_msg_t msg;
        msg.type = queue_msg_t::JS;
        msg.value.js_data = js;

        send(queue_socks[QUEUE_SOCK_OUT], &msg, sizeof(msg), 0);
    }
    else if (cmd == "led")
    {
        unsigned pwm;
        ss >> pwm;
        led_pwm = pwm;
        slave.pwmDuty(1, 1, led_pwm*255/100);
    }
}
