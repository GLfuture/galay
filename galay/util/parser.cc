#include "parser.h"
#include "file.h"

int 
galay::Parser::ConfigParser::parse(const std::string &filename)
{
    std::string buffer = FileOP::ReadFile(filename);
    int ret = ParseContent(buffer);
    return ret;
}

int 
galay::Parser::ConfigParser::ParseContent(const std::string& content)
{
    std::stringstream stream(content);
    std::string line;
    while (std::getline(stream, line))
    {
        ConfType state = ConfType::CONF_KEY;
        std::string key, value;
        for (int i = 0; i < line.length(); i++)
        {
            if (line[i] == ' ')
                continue;
            if (line[i] == '#')
                break;
            if ((line[i] == '=' || line[i] == ':') && state == ConfType::CONF_KEY)
            {
                state = ConfType::CONF_VALUE;
                continue;
            }
            switch (state)
            {
            case CONF_KEY:
                key += line[i];
                break;
            case CONF_VALUE:
                value += line[i];
                break;
            default:
                break;
            }
        }
        m_fields.insert({key, value});
    }
    return 0;
}

std::string_view 
galay::Parser::ConfigParser::get_value(std::string_view key)
{
    auto it = m_fields.find(std::string(key));
    if (it == m_fields.end())
        return "";
    return it->second;
}

std::string 
galay::Parser::ConfigParser::get_value(const std::string &key)
{
    auto it = m_fields.find(key);
    if (it == m_fields.end())
        return "";
    return it->second;
}