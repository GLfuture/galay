#ifndef GALAY_PARSER_H
#define GALAY_PARSER_H

#include <string>

namespace galay{
    class Parser
    {
    public:
        virtual int parse(const std::string& filename) = 0;
        virtual int parse(const char* content , uintmax_t len) = 0;
    };
}

#endif