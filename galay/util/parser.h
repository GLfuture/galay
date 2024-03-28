#ifndef GALAY_PARSER_H
#define GALAY_PARSER_H

#include <string>

namespace galay{
    class Parser
    {
    public:
        virtual int parse(const std::string& filename) = 0;
    protected:    
        virtual int ParseContent(const std::string& content) = 0;
    };
}

#endif