#include "ws_server_isolated.hpp"

#include "ws_server.hpp"
#include "ws_broadcaster.hpp"

#include <memory>

namespace ws
{

std::unique_ptr<ws_server> g_ws_server;
std::unique_ptr<ws_broadcaster> g_ws_broadcaster;
std::thread ws_server_run;

int ws_server_queue_socks[2];

int ws_send(uint32_t id, const std::string& msg)
{
    return g_ws_server->send(id, msg);
}

void ws_server_init()
{
    if (0 > socketpair(AF_UNIX, SOCK_DGRAM, 0, ws_server_queue_socks))
    {
        std::stringstream ss;
        ss << "Failed to create socketpair! Error=";
        ss << strerror(errno);
        throw std::runtime_error(ss.str());
    }

    g_ws_server = std::make_unique<ws_server>();
    g_ws_broadcaster = std::make_unique<ws_broadcaster>();
}

} // namespace ws
