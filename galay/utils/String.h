#ifndef __GALAY_STRING_H__
#define __GALAY_STRING_H__

#include <string>
#include <vector>
#include <cinttypes>
#if __cplusplus >= 201703L
#include <string_view>
#endif
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace galay::utils
{
    class StringSplitter
    {
    public:
        // 字符分割
        static std::vector<std::string> SpiltWithChar(const std::string &str, const char symbol);

        // 字符串分割
        static std::vector<std::string> SpiltWithStr(const std::string &str, const std::string &symbol);

        // 用一个字符分割字符串，单引号中的内容视为一个整体，\'进行转义
        static inline std::vector<std::string> SpiltWithCharConnectWithQuote(const std::string &str, const char partition)
        {
            return SpiltWithCharAndConnectWithchar(str, partition, '\'');
        }

        // 用一个字符分割字符串，双引号中的内容视为一个整体，\"进行转移
        static inline std::vector<std::string> SpiltWithCharConnectWithDoubleQuote(const std::string &str, const char partition)
        {
            return SpiltWithCharAndConnectWithchar(str, partition, '\"');
        }

#if __cplusplus >= 201703L
        static std::vector<std::string_view> SpiltWithChar(std::string_view str, const char symbol);
        static std::vector<std::string_view> SpiltWithStr(std::string_view str, std::string_view symbol);
        static inline std::vector<std::string_view> SpiltWithCharConnectWithQuote(std::string_view str, const char partition)
        {
            return SpiltWithCharAndConnectWithchar(str, partition, '\'');
        }
        static inline std::vector<std::string_view> SpiltWithCharConnectWithDoubleQuote(std::string_view str, const char partition)
        {
            return SpiltWithCharAndConnectWithchar(str, partition, '\"');
        }
#endif

    protected:
        static std::vector<std::string> SpiltWithCharAndConnectWithchar(const std::string &str, const char partition, const char connction);
#if __cplusplus >= 201703L
        static std::vector<std::string_view> SpiltWithCharAndConnectWithchar(std::string_view str, const char partition, const char connction);
#endif
    };

    class StringCounter
    {
    public:
        static inline uint32_t Count(const std::string &str, const char symbol)
        {
            return std::count(str.begin(), str.end(), symbol);
        }

        static inline uint32_t Count(const std::string &str, const std::string &symbol)\
        {
            return std::count(str.begin(), str.end(), symbol[0]);
        }
    };

    inline std::string uint8ToString(const std::vector<uint8_t>& data) {
        std::ostringstream oss;
        for (uint8_t byte : data) {
            oss << static_cast<char>(byte);
        }
        return oss.str();
    }

    inline std::string uint8ToVisibleHex(const std::vector<uint8_t>& data) {
        std::ostringstream oss;
        oss << std::hex << std::setw(2) << std::setfill('0');
        bool first = true;
        for (uint8_t byte : data) {
            if (!first) {
                oss << " ";
            }
            oss << static_cast<int>(byte);
            first = false;
        }
        return oss.str();
    }
    
    inline std::vector<uint8_t> stringToUint8(const std::string& data) {
        std::vector<uint8_t> result;
        result.reserve(data.length());
        for (size_t i = 0; i < data.length(); ++i) {
            result.push_back(static_cast<uint8_t>(data[i]));
        }
        return result;
    }
    
    inline std::vector<uint8_t> stringviewToUint8(std::string_view data) {
        std::vector<uint8_t> result;
        result.reserve(data.length());
        for (size_t i = 0; i < data.length(); ++i) {
            result.push_back(static_cast<uint8_t>(data[i]));
        }
        return result;
    }
    
}

#endif