#ifndef __GALAY_SHA256_H__
#define __GALAY_SHA256_H__


#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>
#if __cplusplus >= 201703L
#include <string_view>
#endif 
#include <openssl/evp.h>
#include <openssl/sha.h>

namespace galay::algorithm
{
    
    class Sha256Util
    {
    public:
        static std::string Encode(const std::string &str);
#if __cplusplus >= 201703L
        static std::string Encode(std::string_view str);
#endif
    };

}

#endif