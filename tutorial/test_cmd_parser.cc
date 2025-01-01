#include "galay/utils/ArgsParse.hpp"
#include <iostream>
int main(int argc, char* argv[])
{
    args::AppCmds app("docker", "such as docker's command");
    app.AddOption("-v,--version", "show version", [](args::Command* app, const std::string& input) {
        std::cout << input << std::endl;
        std::cout << "version: 2" << std::endl;
    });
    app.AddOption("-u,--use", "use tools", [](args::Command* app, const std::string& input) {
        std::cout << "toools: " << input << std::endl;
    });
    return 0;
}