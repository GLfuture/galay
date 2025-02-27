#include "galay/utils/App.hpp"
#include <iostream>
int main(int argc, const char* argv[])
{
    galay::args::App app("tutorial_cmd_parser");
    galay::args::Arg* arg = galay::args::Arg::Create("version"), *arg2 = galay::args::Arg::Create("input");
    arg->Short('v').Output("Version 2.0");
    arg2->Short('i').Input(true).IsString();
    app.AddArg(arg, true).Help("./tutorial_cmd_parser [OPTIONS]\n   -v, --versopn\t\tshow version");
    if(!app.Parse(argc, argv)) {
        app.ShowHelp();
        return -1;
    }
    std::string input = arg2->Value().ConvertTo<std::string>();
    return 0;
}