#ifndef __LOG_HPP__
#define __LOG_HPP__

#include <cstdio>
#include <chrono>
#include <sstream>
#include <thread>

template <typename... Args>
void LOG(const char* fmt, Args&&... args)
{
    char buff[1024*16];
    sprintf(buff, fmt, std::forward<Args&&>(args)...);

    auto us_epoch = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    std::stringstream ss;
    ss << us_epoch << " | THR" << std::this_thread::get_id() << " | " << buff;
    std::printf(ss.str().c_str());
    std::fflush(stdout);
}

#endif // __LOG_HPP__
