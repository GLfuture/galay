#include "conf_parser.h"
#include "file.h"

int galay::Conf_Parser::parse(const std::string &filename)
{
    std::string buffer = FileOP::ReadFile(filename);
    int ret = ParseContent(buffer);
    return ret;
}

int galay::Conf_Parser::ParseContent(const std::string& content)
{
    std::stringstream stream(content);
    std::string line;
    while (std::getline(stream, line))
    {
        Conf_Type state = Conf_Type::CONF_KEY;
        std::string key, value;
        for (int i = 0; i < line.length(); i++)
        {
            if (line[i] == ' ')
                continue;
            if (line[i] == '#')
                break;
            if ((line[i] == '=' || line[i] == ':') && state == Conf_Type::CONF_KEY)
            {
                state = Conf_Type::CONF_VALUE;
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

std::string_view galay::Conf_Parser::get_value(std::string_view key)
{
    auto it = m_fields.find(std::string(key));
    if (it == m_fields.end())
        return "";
    return it->second;
}

std::string galay::Conf_Parser::get_value(const std::string &key)
{
    auto it = m_fields.find(key);
    if (it == m_fields.end())
        return "";
    return it->second;
}