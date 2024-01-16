#ifndef __WS_SERVER_BROADCASTER_HPP__
#define __WS_SERVER_BROADCASTER_HPP__

#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <bfc/Udp.hpp>
#include <bfc/Buffer.hpp>

#include <map>

namespace ws
{

using server_t = websocketpp::server<websocketpp::config::asio>;
using message_ptr_t = server_t::message_ptr;

class ws_broadcaster
{
public:
    ws_broadcaster();
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, message_ptr_t msg);
    void run_ws();
    void run_udp();

private:
    bfc::UdpSocket udp_sock;
    std::thread run_udp_thread;

    server_t server;
    std::thread run_ws_thread;

    std::mutex server_mutex;
    uint32_t connection_id_ctr = 0;
    std::map<uint32_t, websocketpp::connection_hdl> connections_fmap;
    std::map<void*, uint32_t> connections_rmap;
};

} // namespace ws

#endif // __WS_SERVER_HPP__
