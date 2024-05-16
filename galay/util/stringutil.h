#ifndef GALAY_STRINGUTIL_H
#define GALAY_STRINGUTIL_H

#include <string>
#include <vector>
#if __cplusplus >= 201703L
#include <string_view>
#endif

namespace galay
{
    namespace util
    {
        class StringUtil
        {
        public:
            // 字符分割
            static ::std::vector<::std::string> Spilt_With_Char(const ::std::string &str, const char symbol);

            // 字符串分割
            static ::std::vector<::std::string> Spilt_With_Str(const ::std::string &str, const ::std::string &symbol);

            // 用一个字符分割字符串，单引号中的内容视为一个整体，\'进行转义
            static ::std::vector<::std::string> Spilt_With_Char_Connect_With_Quote(const ::std::string &str, const char partition)
            {
                return Spilt_With_Char_Connect_With_char(str, partition, '\'');
            }

            // 用一个字符分割字符串，双引号中的内容视为一个整体，\"进行转移
            static ::std::vector<::std::string> Spilt_With_Char_Connect_With_Double_Quote(const ::std::string &str, const char partition)
            {
                return Spilt_With_Char_Connect_With_char(str, partition, '\"');
            }

#if __cplusplus >= 201703L
            static ::std::vector<::std::string_view> Spilt_With_Char(::std::string_view str, const char symbol);
            static ::std::vector<::std::string_view> Spilt_With_Str(::std::string_view str, ::std::string_view symbol);
            static ::std::vector<::std::string_view> Spilt_With_Char_Connect_With_Quote(::std::string_view str, const char partition)
            {
                return Spilt_With_Char_Connect_With_char(str, partition, '\'');
            }
            static ::std::vector<::std::string_view> Spilt_With_Char_Connect_With_Double_Quote(::std::string_view str, const char partition)
            {
                return Spilt_With_Char_Connect_With_char(str, partition, '\"');
            }
#endif

        protected:
            static ::std::vector<::std::string> Spilt_With_Char_Connect_With_char(const ::std::string &str, const char partition, const char connction);
#if __cplusplus >= 201703L
            static ::std::vector<::std::string_view> Spilt_With_Char_Connect_With_char(::std::string_view str, const char partition, const char connction);
#endif
        };
    }
}

#endif