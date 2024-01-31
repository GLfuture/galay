#ifndef GALAY_CONF_PARSER_H
#define GALAY_CONF_PARSER_H

#include "parser.h"
#include <fstream>
#include <filesystem>
#include <string.h>
#include <sstream>
#include <string_view>
#include <unordered_map>

namespace galay{
    class Conf_Parser: public Parser
    {
        enum Conf_Type{
            CONF_KEY,
            CONF_VALUE,
        };

    public:
        int parse(const std::string& filename) override;

        int parse(const char* content , uintmax_t len) override;

#if __cplusplus >= 201703L
        std::string_view get_value(std::string_view key);
#endif
        std::string get_value(const std::string &key);
    private:
        std::unordered_map<std::string,std::string> m_fields;
    };


}

#endif