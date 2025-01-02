#include "galay/utils/ArgsParse.hpp"
#include <iostream>
int main(int argc, const char* argv[])
{
    args::CmdArgsParser app("docker\n such as docker's command\n  -c <command> [<args>]");
    app.AddOption<args::CmdType::Output>("-v,--version", [](args::SubCommand* app, args::Option* option) {
        std::cout << option->GetValue() << std::endl;
        std::cout << "version: 2" << std::endl;
    });
    app.AddOption<args::CmdType::InputAndOutput>("-u,--use", [](args::SubCommand* app, args::Option* option) {
        std::cout << option->GetValue() << std::endl;
        std::cout << "Hello To Use This Tool" << std::endl;
    });
    app.AddOption<args::CmdType::Output>("-h,--help", [](args::SubCommand* app, args::Option* option) {
    });
    if(app.AppCmdsParse(argc, argv)) {
        app.ExecuteAllParsedCmds();
    }
    std::cout << app.ToString();
    app.CleanUp();
    return 0;
}