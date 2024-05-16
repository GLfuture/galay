#ifndef GALAY_SHA256_H
#define GALAY_SHA256_H


#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>
#if __cplusplus >= 201703L
#include <string_view>
#endif 
#include <openssl/evp.h>
#include <openssl/sha.h>

namespace galay
{
    namespace security
    {
        class Sha256Util
        {
        public:
            static ::std::string encode(const ::std::string &str);
#if __cplusplus >= 201703L
            static ::std::string encode(::std::string_view str);
#endif
        };
    }
}

#endif