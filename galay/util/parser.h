#ifndef GALAY_PARSER_H
#define GALAY_PARSER_H

#include <fstream>
#include <filesystem>
#include <string.h>
#include <sstream>
#include <string_view>
#include <unordered_map>

namespace galay
{
    namespace Parser
    {
        class ParserBase
        {
        public:
            virtual int parse(const std::string &filename) = 0;

        protected:
            virtual int ParseContent(const std::string &content) = 0;
        };

        class ConfigParser : public ParserBase
        {
            enum ConfType
            {
                CONF_KEY,
                CONF_VALUE,
            };

        public:
            int parse(const std::string &filename) override;
#if __cplusplus >= 201703L
            std::string_view get_value(std::string_view key);
#endif
            std::string get_value(const std::string &key);

        private:
            int ParseContent(const std::string &content) override;

        private:
            std::unordered_map<std::string, std::string> m_fields;
        };

    }

}

#endif