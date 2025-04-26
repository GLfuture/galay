#ifndef __GALAY_MD5_H__
#define __GALAY_MD5_H__

#include <openssl/md5.h>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>
#if __cplusplus >= 201703L
#include <string_view>
#endif

namespace galay::algorithm
{
    class Md5Util
    {
    public:
        static std::string Encode(std::string const &str);
#if __cplusplus >= 201703L
        static std::string Encode(std::string_view str);
#endif
    };
}

#endif