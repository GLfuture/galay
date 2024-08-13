#include "parser.h"
#include "fileiofunction.h"
#include "stringutil.h"

galay::util::ParserManager::ParserManager()
{
    RegisterExtension<ConfigParser>(".conf");
    RegisterExtension<JsonParser>(".json");
}


galay::util::ParserBase::ptr 
galay::util::ParserManager::CreateParser(const std::string &filename,bool IsParse)
{
    if(filename.empty()) return nullptr;
    int pos = filename.find_last_of(".");
    if(pos == std::string::npos) return nullptr;
    std::string ext = filename.substr(pos);
    if(!m_creater.contains(ext)) return nullptr;
    auto parser = m_creater[ext]();
    if(IsParse) parser->Parse(filename);
    return parser;
}

int 
galay::util::ConfigParser::Parse(const std::string &filename)
{
    std::string buffer = galay::IOFunction::FileIOFunction::ZeroCopyFile::ReadFile(filename);
    int ret = ParseContent(buffer);
    return ret;
}

int 
galay::util::ConfigParser::ParseContent(const std::string& content)
{
    std::stringstream stream(content);
    std::string line;
    while (std::getline(stream, line))
    {
        ConfType state = ConfType::kConfKey;
        std::string key, value;
        for (int i = 0; i < line.length(); i++)
        {
            if (line[i] == ' ')
                continue;
            if (line[i] == '#')
                break;
            if ((line[i] == '=' || line[i] == ':') && state == ConfType::kConfKey)
            {
                state = ConfType::kConfValue;
                continue;
            }
            switch (state)
            {
            case kConfKey:
                key += line[i];
                break;
            case kConfValue:
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

std::any 
galay::util::ConfigParser::GetValue(const std::string &key)
{
    auto it = m_fields.find(key);
    if (it == m_fields.end())
        return "";
    return it->second;
}


int 
galay::util::JsonParser::Parse(const std::string &filename)
{
    std::string buffer = galay::IOFunction::FileIOFunction::ZeroCopyFile::ReadFile(filename);
    if(!nlohmann::json::accept(buffer)){
        return -1;
    }
    this->m_json = nlohmann::json::parse(buffer);
    return 0;
}

std::any 
galay::util::JsonParser::GetValue(const std::string &key)
{
    std::vector<std::string> path = galay::util::StringUtil::SpiltWithChar(key, '.');
    nlohmann::json j;
    for(auto &p : path){
        if(this->m_json.contains(p)){
            j = this->m_json[p];
        }else return {};
    }
    if(j.is_string()){
        return j.get<std::string>();
    }else if(j.is_number()){
        return j.get<int>();
    }else if(j.is_boolean()){
        return j.get<bool>();
    }else if(j.is_array()){
        return j;
    }else if(j.is_object()){
        return j;
    }
    return {};
}