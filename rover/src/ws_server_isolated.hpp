#ifndef __WS_SERVER_ISOLATED_HPP__
#define __WS_SERVER_ISOLATED_HPP__

#include <cstdint>
#include <string>

namespace ws
{

extern void ws_server_init();
extern int ws_send(uint32_t, const std::string&);

struct queue_msg_t
{
    uint32_t conn_id;
    char* data_str;
};

enum {QUEUE_SOCK_USER, QUEUE_SOCK_SERVER};
extern int ws_server_queue_socks[2];

} // namespace ws


#endif // __WS_SERVER_ISOLATED_HPP__
