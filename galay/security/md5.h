#ifndef GALAY_MD5_H
#define GALAY_MD5_H

#include <openssl/md5.h>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>
#if __cplusplus >= 201703L
#include <string_view>
#endif

namespace galay
{
    namespace security
    {
        class Md5Util
        {
        public:
            static std::string encode(std::string const &str);
#if __cplusplus >= 201703L
            static std::string encode(std::string_view str);
#endif
        };

    }
}

#endif