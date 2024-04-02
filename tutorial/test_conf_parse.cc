#include "../galay/util/parser.h"
#include <iostream>

int main()
{
    galay::Helper::ConfigParser parse;
    parse.parse("a.conf");
    std::cout << parse.get_value(std::string("ip")) << ":" << parse.get_value(std::string("port")) << "/" \
    << parse.get_value(std::string("name")) << '\n';
    return 0;
}