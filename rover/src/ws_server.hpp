#ifndef __WS_SERVER_HPP__
#define __WS_SERVER_HPP__

#include "ws_server_isolated.hpp"

#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <map>

namespace ws
{

using server_t = websocketpp::server<websocketpp::config::asio>;
using message_ptr_t = server_t::message_ptr;

class ws_server
{
public:
    ws_server();
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, message_ptr_t msg);
    void run();

    int send(uint32_t, const std::string&);
    websocketpp::connection_hdl get_handle(uint32_t);
private:
    server_t server;
    std::thread run_thread;

    std::mutex server_mutex;
    uint32_t connection_id_ctr = 0;
    std::map<uint32_t, websocketpp::connection_hdl> connections_fmap;
    std::map<void*, uint32_t> connections_rmap;
};

} // namespace ws

#endif // __WS_SERVER_HPP__
