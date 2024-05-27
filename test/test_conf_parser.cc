#include "../galay/util/parser.h"
#include <iostream>

int main(int argc, char *argv[])
{
    galay::util::ConfigParser parser;
    parser.parse("init.conf");
    std::cout << parser.get_value(std::string("author")) << std::endl;
    return 0;
}