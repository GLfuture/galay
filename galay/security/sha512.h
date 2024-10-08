#ifndef GALAY_SHA512_H
#define GALAY_SHA512_H

#include <string>
#include <openssl/sha.h>

namespace galay::security
{
    class Sha512Util{
    public:
        static std::string encode(const std::string& str);
    };
}



#endif