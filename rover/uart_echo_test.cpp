#include <errno.h>

#include <iomanip>
#include <thread>
#include <cstring>
#include <sstream>

#include <wiringSerial.h>

int main()
{
    int fd;

    if ((fd = serialOpen ("/dev/serial0", 115200)) < 0)
    {
        fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
        return 1 ;
    }

    auto sender = std::thread([&](){
        uint32_t count = 0;
        while (true)
        {
            std::stringstream ss;
            ss << std::setw(10) << std::setfill('0') << count << ";";
            serialPuts(fd, ss.str().c_str());
            std::this_thread::sleep_for(std::chrono::microseconds(75));
            std::printf("send: %s\n", ss.str().c_str());
            count++;
        }
    });


    auto receiver = std::thread([&](){
        uint8_t buff[1024];
        size_t buff_idx=0;
        while (true)
        {
            while (serialDataAvail(fd))
            {
                auto cdata = serialGetchar(fd);
                buff[buff_idx++] = cdata;
                buff[buff_idx] = 0;
                if (cdata == ';')
                {
                    std::printf("recv: %s\n", buff);
                    buff_idx = 0;
                    buff[buff_idx] = 0;
                }
            }
        }
    });

    sender.join();
    receiver.join();
}