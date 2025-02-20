#include "galay/utils/App.hpp"
#include <iostream>
int main(int argc, const char* argv[])
{
    args::App app("docker\n such as docker's command\n  -c <command> [<args>]");
    args::Arg* arg = args::Arg::Create("version");
    arg->Short("v").Output("Version 2.0");
    app.AddArg(arg, true);
    app.Parse(argc, argv);
    return 0;
}