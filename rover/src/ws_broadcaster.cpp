#include "ws_broadcaster.hpp"

#include "Log.hpp"

#include <sstream>
#include <cstdio>
#include <iomanip>

using namespace ws;

static std::string to_hex_string(const void* data, size_t size)
{
    auto cdata = (const char*) data;

    std::stringstream ss;
    ss << "data[" << size << "]={";
    for (size_t i=0; i<size; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << int(cdata[i]);
    }
    ss << "}";
    return ss.str();
}

ws_broadcaster::ws_broadcaster()
{
    run_ws_thread = std::thread([this](){run_ws();});
    run_udp_thread = std::thread([this](){run_udp();});
}

void ws_broadcaster::on_message(websocketpp::connection_hdl hdl, message_ptr_t msg)
{
    // @note : do nothing
}

void ws_broadcaster::on_open(websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> lg(server_mutex);
    auto id = connection_id_ctr++;
    connections_fmap.emplace(id, hdl);
    connections_rmap.emplace(hdl.lock().get(), id);
}

void ws_broadcaster::on_close(websocketpp::connection_hdl hdl)
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

// int ws_server::send(uint32_t id, const std::string& message)
// {
//     std::unique_lock<std::mutex> lg(server_mutex);
//     auto fmap_it = connections_fmap.find(id);
//     if (fmap_it != connections_fmap.end())
//     {
//         server.send(fmap_it->second, message, websocketpp::frame::opcode::text);
//         return message.size();
//     }
//     return -1;
// }

void ws_broadcaster::run_udp()
{
    udp_sock.bind(bfc::IpPort(0, 9000));

    std::byte rcv_raw_buffer[1024*64];
    auto rcv_buffer = bfc::BufferView(rcv_raw_buffer, sizeof(rcv_raw_buffer));
    bfc::IpPort sender;
    while (true)
    {
        auto sz = udp_sock.recvfrom(rcv_buffer,sender);
        // auto rcvd_raw_str = to_hex_string(rcv_raw_buffer, sz);
        // LOG("ws_server::run_udp | received %s.\n", rcvd_raw_str.c_str());
        std::unique_lock<std::mutex> lg(server_mutex);
        for (auto& i : connections_fmap)
        {
            // LOG("ws_server::run_udp | forward to conn %d.\n", i.first);
            server.send(i.second, (char*)rcv_raw_buffer, sz, websocketpp::frame::opcode::binary);
        }
    }
}

void ws_broadcaster::run_ws()
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

        server.listen(9003);
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
