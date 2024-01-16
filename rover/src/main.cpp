#include <iostream>
#include <regex>

#include "app.hpp"
#include "ws_server_isolated.hpp"

int main(int argc, const char* argv[])
{
    std::regex arger("^--(.+?)=(.+?)$");
    std::smatch match;
    app::options_t options;

    for (int i=1; i<argc; i++)
    {
        auto s = std::string(argv[i]);
        if (std::regex_match(s, match, arger))
        {
            options.emplace(match[1].str(), match[2].str());
        }
        else
        {
            throw std::runtime_error(std::string("invalid argument: `") + argv[i] + "`");
        }
    }

    ws::ws_server_init();

    return app::main_app(options).run();
}
