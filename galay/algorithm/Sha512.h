#ifndef __GALAY_SHA512_H__
#define __GALAY_SHA512_H__

#include <string>
#include <openssl/sha.h>

namespace galay::algorithm
{
    class Sha512Util{
    public:
        static std::string encode(const std::string& str);
    };
}



#endif