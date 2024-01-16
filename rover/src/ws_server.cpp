#include "ws_server_isolated.hpp"
#include "ws_server.hpp"

#include "Log.hpp"

#include <sstream>
#include <cstdio>

using namespace ws;

ws_server::ws_server()
{
    run_thread = std::thread([this](){run();});
}

void ws_server::on_message(websocketpp::connection_hdl hdl, message_ptr_t msg)
{
    auto conn_ptr = hdl.lock().get();
    auto rmap_it = connections_rmap.find(conn_ptr);
    if (rmap_it == connections_rmap.end())
    {
        LOG("ws_server::on_message | error can find connection %p.\n", conn_ptr);
        return;
    }
    queue_msg_t qmsg;
    qmsg.conn_id = rmap_it->second;
    auto& payload = msg->get_payload();
    qmsg.data_str = new char[payload.size()+1];
    qmsg.data_str[payload.size()] = 0;
    std::memcpy(qmsg.data_str, payload.data(), payload.size());

    ::send(ws_server_queue_socks[QUEUE_SOCK_SERVER], &qmsg, sizeof(msg), 0);
}

void ws_server::on_open(websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> lg(server_mutex);
    auto id = connection_id_ctr++;
    connections_fmap.emplace(id, hdl);
    connections_rmap.emplace(hdl.lock().get(), id);
}

void ws_server::on_close(websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> lg(server_mutex);
    auto rmap_it = connections_rmap.find(hdl.lock().get());
    if (rmap_it == connections_rmap.end())
    {
        return;
    }
    auto id = rmap_it->second;
    auto hdl_ptr = rmap_it->first;
    connections_fmap.erase(id);
    connections_rmap.erase(hdl_ptr);
}

int ws_server::send(uint32_t id, const std::string& message)
{
    std::unique_lock<std::mutex> lg(server_mutex);
    auto fmap_it = connections_fmap.find(id);
    if (fmap_it != connections_fmap.end())
    {
        server.send(fmap_it->second, message, websocketpp::frame::opcode::text);
        return message.size();
    }
    return -1;
}

websocketpp::connection_hdl ws_server::get_handle(uint32_t id)
{
    std::unique_lock<std::mutex> lg(server_mutex);
    auto fmap_it = connections_fmap.find(id);
    if (fmap_it == connections_fmap.end())
    {
        return std::weak_ptr<void>();
    }
    return fmap_it->second;
}

void ws_server::run()
{
    try
    {
        server.set_access_channels(websocketpp::log::alevel::all);
        server.clear_access_channels(websocketpp::log::alevel::frame_payload);
        server.init_asio();

        server.set_message_handler([this](websocketpp::connection_hdl hdl, message_ptr_t msg)
            {
                on_message(hdl, msg);
            }
        );

        server.set_open_handler([this](websocketpp::connection_hdl hdl)
            {
                on_open(hdl);
            }
        );

        server.set_close_handler([this](websocketpp::connection_hdl hdl)
            {
                on_close(hdl);
            }
        );

        server.listen(9002);
        server.start_accept();
        server.run();
    }
    catch (websocketpp::exception const & e)
    {
        std::cout << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "other exception" << std::endl;
    }
}
