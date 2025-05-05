#include "Parser.h"
#include "System.h"
#include "String.h"

namespace galay::parser
{
static void Trim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

ParserManager::ParserManager()
{
    RegisterExtension<ConfigParser>(".conf");
#ifdef USE_NLOHMANN_JSON
    RegisterExtension<JsonParser>(".json");
#endif
}


ParserBase::ptr 
ParserManager::CreateParser(const std::string &filename,bool IsParse)
{
    if(filename.empty()) return nullptr;
    int pos = filename.find_last_of(".");
    if(pos == std::string::npos) return nullptr;
    std::string ext = filename.substr(pos);
    if(!m_creater.contains(ext)) return nullptr;
    auto parser = m_creater[ext]();
    if(IsParse) {
        if(parser->Parse(filename) != 0) {
            return nullptr;
        }
    }
    return parser;
}

int 
ConfigParser::Parse(const std::string &filename)
{
    std::string buffer = galay::utils::ZeroReadFile(filename);
    int ret = ParseContent(buffer);
    return ret;
}

int 
ConfigParser::ParseContent(const std::string& content) {
    std::stringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
        ConfType state = ConfType::kConfKey;
        std::string key, value;
        bool in_array = false;

        for (size_t i = 0; i < line.length(); ++i) {
            char c = line[i];
            if (c == ' ') continue;
            if (c == '#') break;

            // 检测数组开始
            if (c == '[' && state == ConfType::kConfValue) {
                in_array = true;
            }

            // 检测数组结束
            if (c == ']' && in_array) {
                in_array = false;
            }

            if ((c == '=' || c == ':') && state == ConfType::kConfKey) {
                state = ConfType::kConfValue;
                continue;
            }

            switch (state) {
            case ConfType::kConfKey:  key += c;  break;
            case ConfType::kConfValue: value += c; break;
            }
        }

        if (key.empty()) continue;

        // 处理数组值
        if (value.front() == '[' && value.back() == ']') {
            std::vector<std::string> elements = ParseArray(value.substr(1, value.size()-2));
            m_fields[key] = elements;
        }
        // 处理字符串值
        else if (value.front() == '\"' && value.back() == '\"') {
            value = value.substr(1, value.size()-2);
            ParseEscapes(value);
            m_fields[key] = value;
        }
        // 处理普通值
        else {
            m_fields[key] = value;
        }
    }
    return 0;
}

std::vector<std::string> 
ConfigParser::ParseArray(const std::string& arr_str) 
{
    std::vector<std::string> elements;
    std::string current;
    bool in_quotes = false;
    char quote_char = '\0';
    bool escape = false;

    for (char c : arr_str) {
        if (escape) {
            // 处理转义字符
            switch (c) {
                case 'n':  current += '\n'; break;
                case 't':  current += '\t'; break;
                case '"':  current += '"';  break;
                case '\'': current += '\''; break;
                case '\\': current += '\\'; break;
                default:   current += '\\'; current += c; break;
            }
            escape = false;
        } else if (c == '\\') {
            // 开始转义序列
            escape = true;
        } else if ((c == '"' || c == '\'') && !in_quotes) {
            // 进入引号模式
            in_quotes = true;
            quote_char = c;
        } else if (c == quote_char && in_quotes) {
            // 结束引号模式
            in_quotes = false;
            quote_char = '\0';
        } else if (c == ',' && !in_quotes) {
            // 分割元素
            Trim(current);
            if (!current.empty() || in_quotes) {
                elements.push_back(current);
            }
            current.clear();
        } else {
            // 普通字符
            current += c;
        }
    }

    // 处理最后一个元素
    Trim(current);
    if (!current.empty() || in_quotes) {
        elements.push_back(current);
    }

    // 后处理引号内容
    for (auto& elem : elements) {
        if (elem.size() >= 2 && 
           ((elem.front() == '"' && elem.back() == '"') ||
            (elem.front() == '\'' && elem.back() == '\''))) 
        {
            elem = elem.substr(1, elem.size() - 2);
        }
        elem = ParseEscapes(elem);
    }

    return elements;
}


std::any 
ConfigParser::GetValue(const std::string &key)
{
    auto it = m_fields.find(key);
    if (it == m_fields.end())
        return std::any();
    return it->second;
}


std::string 
ConfigParser::ParseEscapes(const std::string& input) {
    std::stringstream ss;
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size()) {
            switch (input[i + 1]) {
                case 'n':  ss << '\n'; break;
                case 't':  ss << '\t'; break;
                case '"':  ss << '"';  break;
                case '\\': ss << '\\'; break;
                default:   ss << input[i] << input[i + 1]; 
            }
            ++i;
        } else {
            ss << input[i];
        }
    }
    return ss.str();
}


#ifdef USE_NLOHMANN_JSON
int 
JsonParser::Parse(const std::string &filename)
{
    std::string buffer = galay::utils::ZeroReadFile(filename);
    if(!nlohmann::json::accept(buffer)){
        return -1;
    }
    this->m_json = nlohmann::json::parse(buffer);
    return 0;
}

std::any 
JsonParser::GetValue(const std::string &key)
{
    std::vector<std::string> path = galay::utils::StringSplitter::SpiltWithChar(key, '.');
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

#endif

}