#ifndef GALAY_PARSER_H
#define GALAY_PARSER_H

#include <fstream>
#include <filesystem>
#include <string.h>
#include <sstream>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>

namespace galay
{
    namespace util
    {
        class ParserBase
        {
        public:
            using ptr = std::shared_ptr<ParserBase>;
            using wptr = std::weak_ptr<ParserBase>;
            using uptr = std::unique_ptr<ParserBase>;
            virtual int Parse(const ::std::string &filename) = 0;
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
            ParserBase::ptr CreateParser(const ::std::string &filename,bool IsParse = true);

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
            int Parse(const ::std::string &filename) override;
            std::any GetValue(const ::std::string &key) override;
        private:
            int ParseContent(const ::std::string &content);
        private:
            ::std::unordered_map<::std::string, ::std::string> m_fields;
        };

#ifdef INCLUDE_NLOHMANN_JSON_HPP_

        using JsonValue = nlohmann::json;
        class JsonParser : public ParserBase
        {
        public:
            int Parse(const ::std::string &filename) override;
            std::any GetValue(const ::std::string &key) override;
        private:
            nlohmann::json m_json;
        };

    }
#endif

}

#endif