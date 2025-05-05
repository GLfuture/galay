#ifndef __GALAY_PARSER_H__
#define __GALAY_PARSER_H__

#include <any>
#include <fstream>
#include <filesystem>
#include <string.h>
#include <sstream>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <functional>

#ifdef INCLUDE_NLOHMANN_JSON_HPP
    #define USE_NLOHMANN_JSON
#endif

#ifdef USE_NLOHMANN_JSON
    #include <nlohmann/json.hpp>
#endif

namespace galay::parser
{
    class ParserBase
    {
    public:
        using ptr = std::shared_ptr<ParserBase>;
        using wptr = std::weak_ptr<ParserBase>;
        using uptr = std::unique_ptr<ParserBase>;
        virtual int Parse(const std::string &filename) = 0;
        virtual std::any GetValue(const std::string& key) = 0;
    };

    template <typename T>
    concept ParserFromBase = std::is_base_of_v<ParserBase, T>;

    class ParserManager{
    public:
        using ptr = std::shared_ptr<ParserManager>;
        using uptr = std::unique_ptr<ParserManager>;
        using wptr = std::weak_ptr<ParserManager>;
        ParserManager();

        // create Parser And call Parser's Parse function
        // arg: filename 1.a file 2.a extension such as ".json"
        // arg: IsParse true: call Parser's Parse function , false: not call (if you just want to create Parser, please set IsParse false)
        // fail: return nullptr  success: return ParserBase::ptr
        ParserBase::ptr CreateParser(const std::string &filename,bool IsParse = true);

        template<ParserFromBase T>
        void RegisterExtension(const std::string& ext)
        {
            m_creater[ext] = []()->ParserBase::ptr{
                return std::make_shared<T>();
            };
        }
    private:
        std::unordered_map<std::string,std::function<ParserBase::ptr()>> m_creater;
    };

    class ConfigParser : public ParserBase
    {
        enum ConfType
        {
            kConfKey,
            kConfValue,
        };

    public:
        int Parse(const std::string &filename) override;
        std::any GetValue(const std::string &key) override;
        template<typename T>
        T GetValueAs(const std::string& key) {
            try {
                return std::any_cast<T>(GetValue(key));
            } catch (...) {
                return T();
            }
        }
    private:
        int ParseContent(const std::string &content);
        std::string ParseEscapes(const std::string& input);
        std::vector<std::string> ParseArray(const std::string& arr_str);
    private:
        std::unordered_map<std::string, std::any> m_fields;
    };

#ifdef USE_NLOHMANN_JSON

    using JsonValue = nlohmann::json;
    class JsonParser : public ParserBase
    {
    public:
        int Parse(const std::string &filename) override;
        std::any GetValue(const std::string &key) override;
    private:
        nlohmann::json m_json;
    };


#endif

}

#endif